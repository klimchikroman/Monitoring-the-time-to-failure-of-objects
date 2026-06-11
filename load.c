#include "load.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 1024

static void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') str[len - 1] = '\0';
}

static int is_blank_line(const char *str) {
    while (*str) {
        if (!isspace((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

void log_error(const char *filename, int line_num, const char *message) {
    fprintf(stderr, "ОШИБКА: %s строка %d: %s\n", filename, line_num, message);
}

static int type_exists(const Type *types, int types_count, int type_id) {
    for (int i = 0; i < types_count; i++) {
        if (types[i].id == type_id) return 1;
    }
    return 0;
}

int load_types(const char *filename, Type **types, int *count) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось открыть файл %s\n", filename);
        return 0;
    }
    
    *types = NULL;
    *count = 0;
    char line[MAX_LINE_LEN];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        trim_newline(line);
        if (is_blank_line(line)) continue;
        
        Type t;
        char name[64];
        if (sscanf(line, "%d %63s %lf %lf %lf", &t.id, name, &t.daily_hours,
                   &t.design_lifetime, &t.design_defect_rate) != 5) {
            log_error(filename, line_num, "Неверный формат строки, пропущено");
            continue;
        }
        
        if (t.daily_hours < 0 || t.design_lifetime <= 0 || t.design_defect_rate < 0) {
            log_error(filename, line_num, "Некорректные значения (отрицательные или ноль), пропущено");
            continue;
        }
        
        strcpy(t.name, name);
        
        Type *new_arr = realloc(*types, (*count + 1) * sizeof(Type));
        if (!new_arr) {
            fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось выделить память\n");
            fclose(f);
            free(*types);
            return 0;
        }
        *types = new_arr;
        (*types)[*count] = t;
        (*count)++;
    }
    
    fclose(f);
    
    if (*count == 0) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не загружено ни одного типа из файла %s\n", filename);
        return 0;
    }
    
    return 1;
}

int load_objects(const char *filename, Object **objects, int *count,
                 const Type *types, int types_count) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось открыть файл %s\n", filename);
        return 0;
    }
    
    *objects = NULL;
    *count = 0;
    char line[MAX_LINE_LEN];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        trim_newline(line);
        if (is_blank_line(line)) continue;
        
        Object obj;
        char name[64];
        int status;
        char start_str[16], end_str[16];
        
        if (sscanf(line, "%d %63s %d %d %15s %15s", &obj.id, name, &obj.type_id,
                   &status, start_str, end_str) != 6) {
            log_error(filename, line_num, "Неверный формат строки, пропущено");
            continue;
        }
        
        if (!type_exists(types, types_count, obj.type_id)) {
            log_error(filename, line_num, "ID типа не существует, пропущено");
            continue;
        }
        
        if (status < -1 || status > 2) {
            log_error(filename, line_num, "Неверный статус (должен быть -1,0,1,2), пропущено");
            continue;
        }
        
        obj.status = status;
        strcpy(obj.name, name);
        
        if (status == -1) {
            obj.start_date = (Date){0, 0, 0};
            obj.end_date = (Date){0, 0, 0};
        } else {
            if (!parse_date(start_str, &obj.start_date)) {
                log_error(filename, line_num, "Неверная дата начала, пропущено");
                continue;
            }
            
            if (status == 0) {
                obj.end_date = obj.start_date;
            } else {
                if (!parse_date(end_str, &obj.end_date)) {
                    log_error(filename, line_num, "Неверная дата окончания, пропущено");
                    continue;
                }
                if (compare_dates(&obj.start_date, &obj.end_date) > 0) {
                    log_error(filename, line_num, "Дата начала позже даты окончания, пропущено");
                    continue;
                }
            }
        }
        
        obj.operating_time = 0.0;
        
        Object *new_arr = realloc(*objects, (*count + 1) * sizeof(Object));
        if (!new_arr) {
            fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось выделить память\n");
            fclose(f);
            free(*objects);
            return 0;
        }
        *objects = new_arr;
        (*objects)[*count] = obj;
        (*count)++;
    }
    
    fclose(f);
    
    if (*count == 0) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не загружено ни одного объекта из файла %s\n", filename);
        return 0;
    }
    
    return 1;
}

void free_types(Type *types) {
    free(types);
}

void free_objects(Object *objects) {
    free(objects);
}
