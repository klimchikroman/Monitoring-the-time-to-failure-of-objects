#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "dates.h"
#include "load.h"
#include "calc.h"
#include "report.h"

#define TYPES_FILE "types.txt"
#define OBJECTS_FILE "objects.txt"
#define REPORT_FILE "report.txt"

static int input_current_date(Date *date) {
    char input[32];
    printf("Введите текущую дату (ДД.ММ.ГГГГ): ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }
    input[strcspn(input, "\n")] = '\0';
    return parse_date(input, date);
}

int main(void) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    Type *types = NULL;
    Object *objects = NULL;
    int types_count = 0;
    int objects_count = 0;
    Date current_date;
    
    printf("=== СИСТЕМА МОНИТОРИНГА НАДЁЖНОСТИ ОБОРУДОВАНИЯ ===\n\n");
    
    if (!load_types(TYPES_FILE, &types, &types_count)) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось загрузить типы оборудования\n");
        return 1;
    }
    printf("Загружено типов: %d\n", types_count);
    
    if (!load_objects(OBJECTS_FILE, &objects, &objects_count, types, types_count)) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось загрузить объекты\n");
        free_types(types);
        return 1;
    }
    printf("Загружено объектов: %d\n\n", objects_count);
    
    while (!input_current_date(&current_date)) {
        fprintf(stderr, "Неверный формат даты. Используйте ДД.ММ.ГГГГ\n");
    }
    
    calculate_all_operating_times(objects, objects_count, &current_date, types, types_count);
    
    TypeStatistics *stats = malloc(types_count * sizeof(TypeStatistics));
    if (!stats) {
        fprintf(stderr, "КРИТИЧЕСКАЯ ОШИБКА: Не удалось выделить память\n");
        free_types(types);
        free_objects(objects);
        return 1;
    }
    
    for (int i = 0; i < types_count; i++) {
        compute_type_statistics(types[i].id, objects, objects_count, types, types_count, 
                                &current_date, &stats[i]);
        compute_exponential_statistics(&stats[i], &types[i]);
    }
    
    generate_report(types, types_count, objects, objects_count, stats, &current_date, REPORT_FILE);
    
    free_types(types);
    free_objects(objects);
    free(stats);
    
    printf("\nГотово.\n");
    return 0;
}
