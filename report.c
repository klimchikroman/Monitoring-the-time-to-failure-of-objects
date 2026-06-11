#include "report.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int get_display_status(const Object *obj, const Date *current_date) {
    if (compare_dates(&obj->start_date, current_date) > 0) {
        return -1;
    }

    if (obj->status == 2 && compare_dates(&obj->end_date, current_date) > 0) {
        return 0;
    }
    
    return obj->status;
}

static const char* status_to_string(int status) {
    switch (status) {
        case -1: return "НЕ ЭКСПЛУАТИРУЕТСЯ";
        case 0: return "РАБОТАЕТ";
        case 1: return "СНЯТ";
        case 2: return "ОТКАЗАЛ";
        default: return "НЕИЗВЕСТНО";
    }
}

void generate_report(const Type *types, int types_count,
                     const Object *objects, int objects_count,
                     const TypeStatistics *stats,
                     const Date *current_date,
                     const char *report_filename) {
    
    FILE *f = fopen(report_filename, "w");
    if (!f) {
        fprintf(stderr, "ОШИБКА: Не удалось создать файл отчёта %s\n", report_filename);
        return;
    }
    
    char date_str[16];
    print_date(current_date, date_str, sizeof(date_str));
    
    fprintf(f, "================================================================================\n");
    fprintf(f, "ОТЧЁТ ПО МОНИТОРИНГУ НАДЁЖНОСТИ ОБОРУДОВАНИЯ\n");
    fprintf(f, "МОДЕЛЬ: Экспоненциальный закон распределения\n");
    fprintf(f, "Дата расчёта: %s\n", date_str);
    fprintf(f, "================================================================================\n\n");
    
    for (int t = 0; t < types_count; t++) {
        const Type *type = &types[t];
        const TypeStatistics *stat = &stats[t];
        
        fprintf(f, "ТИП: %d (%s)\n", type->id, type->name);
        fprintf(f, "  Ежедневная наработка: %.1f часов\n", type->daily_hours);
        fprintf(f, "  Расчётный ресурс: %.1f часов\n", type->design_lifetime);
        fprintf(f, "  Допустимый процент брака: %.1f%%\n", type->design_defect_rate);
        fprintf(f, "--------------------------------------------------------------------------------\n");
        
        int has_objects = 0;
        for (int i = 0; i < objects_count; i++) {
            if (objects[i].type_id == type->id) {
                has_objects = 1;
                break;
            }
        }
        
        if (!has_objects) {
            fprintf(f, "  Объектов данного типа не найдено.\n");
        } else {
            fprintf(f, "  Объекты:\n");
            for (int i = 0; i < objects_count; i++) {
                if (objects[i].type_id != type->id) continue;
                
                int display_status = get_display_status(&objects[i], current_date);
                
                fprintf(f, "    ID: %d, Название: %s, Состояние: %s, Наработка: %.1f часов\n",
                        objects[i].id, objects[i].name,
                        status_to_string(display_status),
                        objects[i].operating_time);
                
                if (display_status == 2 && objects[i].operating_time < type->design_lifetime) {
                    fprintf(f, "      ПРЕДУПРЕЖДЕНИЕ: Отказ до расчётного ресурса (%.1f < %.1f)\n",
                            objects[i].operating_time, type->design_lifetime);
                }
            }
            
            fprintf(f, "\n  Статистика по типу %d:\n", type->id);
            
            if (stat->total > 0) {
                fprintf(f, "    Всего объектов (статистически значимых): %d\n", stat->total);
                fprintf(f, "    Работает: %d\n", stat->operational);
                fprintf(f, "    Снято без отказа: %d\n", stat->removed);
                fprintf(f, "    Отказало: %d\n", stat->failed);
                fprintf(f, "    Брак (отказ до расчётного ресурса): %d\n", stat->defective);
                fprintf(f, "    Процент брака: %.1f%%\n", stat->defect_rate);
                
                if (stat->defect_rate > type->design_defect_rate) {
                    fprintf(f, "      ПРЕДУПРЕЖДЕНИЕ: Процент брака (%.1f%%) превышает допустимый (%.1f%%)\n",
                            stat->defect_rate, type->design_defect_rate);
                }
            } else {
                fprintf(f, "    Нет объектов, пригодных для статистического анализа.\n");
            }
            
            fprintf(f, "\n  ЭКСПОНЕНЦИАЛЬНАЯ МОДЕЛЬ (постоянная интенсивность отказов):\n");
            
            if (stat->lambda_valid && stat->failed > 0) {
                fprintf(f, "    Интенсивность отказов λ = %.2e 1/час\n", stat->lambda);
                fprintf(f, "    Средняя наработка до отказа СН = 1/λ = %.1f часов\n", stat->mttf);
                
                if (stat->mttf < type->design_lifetime) {
                    fprintf(f, "      ПРЕДУПРЕЖДЕНИЕ: СН (%.1f) меньше расчётного ресурса (%.1f)\n",
                            stat->mttf, type->design_lifetime);
                }
                
                fprintf(f, "\n    --- ПРОГНОЗ ОТКАЗОВ ---\n");
                if (stat->operational > 0) {
                    fprintf(f, "    Количество работающих объектов: %d\n", stat->operational);
                    fprintf(f, "    Прогнозируемое число отказов:\n");
                    fprintf(f, "      - на 3 месяца: %.1f\n", stat->predicted_failures_3m);
                    fprintf(f, "      - на 6 месяцев: %.1f\n", stat->predicted_failures_6m);
                    fprintf(f, "      - на 12 месяцев: %.1f\n", stat->predicted_failures_12m);
                    
                    if (stat->predicted_failures_12m > stat->operational * 0.5) {
                        fprintf(f, "      ВНИМАНИЕ: Высокий риск отказов (>50%% работающих объектов за год)\n");
                    } else if (stat->predicted_failures_12m > stat->operational * 0.3) {
                        fprintf(f, "      ВНИМАНИЕ: Средний риск отказов (30-50%% за год)\n");
                    } else {
                        fprintf(f, "      Низкий риск отказов (<30%% за год)\n");
                    }
                } else {
                    fprintf(f, "    Нет работающих объектов для прогноза\n");
                }
                
                fprintf(f, "\n    --- РЕКОМЕНДУЕМЫЙ МЕЖРЕМОНТНЫЙ ПЕРИОД ---\n");
                fprintf(f, "    Целевая вероятность отказа за период: 5%%\n");
                if (stat->recommended_tbo > 0 && stat->recommended_tbo < type->design_lifetime) {
                    fprintf(f, "    Рекомендуемый межремонтный период (МРП): %.0f часов\n", stat->recommended_tbo);
                    double days_tbo = stat->recommended_tbo / type->daily_hours;
                    fprintf(f, "    (≈ %.1f дней при %.1f часах/день)\n", days_tbo, type->daily_hours);
                    
                    if (stat->recommended_tbo < type->design_lifetime * 0.5) {
                        fprintf(f, "      ВНИМАНИЕ: МРП менее 50%% конструкторского ресурса\n");
                        fprintf(f, "      Рекомендуется усилить контроль качества\n");
                    }
                } else if (stat->recommended_tbo >= type->design_lifetime) {
                    fprintf(f, "    Рекомендуемый МРП = конструкторский ресурс (%.0f часов)\n", type->design_lifetime);
                    fprintf(f, "    Оборудование работает в соответствии с ожиданиями\n");
                }
                
                double prob_at_design = 1.0 - exp(-stat->lambda * type->design_lifetime);
                fprintf(f, "\n    --- ДОПОЛНИТЕЛЬНЫЕ РАСЧЁТЫ ---\n");
                fprintf(f, "    Вероятность отказа за конструкторский ресурс (%.0f ч): %.1f%%\n",
                        type->design_lifetime, prob_at_design * 100.0);
                
                if (prob_at_design > 0.1) {
                    fprintf(f, "      ЗАМЕЧАНИЕ: Значительная вероятность отказа (%d%%) за паспортный ресурс\n",
                            (int)(prob_at_design * 100.0));
                }
                
            } else if (stat->lambda_valid && stat->failed == 0) {
                fprintf(f, "    Отказов не зафиксировано.\n");
                fprintf(f, "    λ ≈ 0 (сверхнадёжное оборудование)\n");
                fprintf(f, "    СН не определён (нет отказов)\n");
                fprintf(f, "    Рекомендуемый МРП = конструкторский ресурс (%.0f часов)\n", type->design_lifetime);
            } else {
                fprintf(f, "    Недостаточно данных для экспоненциального анализа (нет отказов)\n");
            }
        }
        
        fprintf(f, "\n================================================================================\n\n");
    }
    
    fprintf(f, "КОНЕЦ ОТЧЁТА\n");
    fclose(f);
    
    printf("Отчёт сохранён в %s\n", report_filename);
}
