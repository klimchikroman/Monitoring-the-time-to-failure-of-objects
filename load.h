#ifndef LOAD_H
#define LOAD_H

#include "dates.h"

typedef struct {
    int id;
    char name[64];
    double daily_hours;
    double design_lifetime;
    double design_defect_rate;
} Type;

typedef struct {
    int id;
    char name[64];
    int type_id;
    Date start_date;
    int status;
    Date end_date;
    double operating_time;
} Object;

int load_types(const char *filename, Type **types, int *count);
int load_objects(const char *filename, Object **objects, int *count,
                 const Type *types, int types_count);
void free_types(Type *types);
void free_objects(Object *objects);
void log_error(const char *filename, int line_num, const char *message);

#endif
