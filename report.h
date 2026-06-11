#ifndef REPORT_H
#define REPORT_H

#include "dates.h"
#include "load.h"
#include "calc.h"

void generate_report(const Type *types, int types_count,
                     const Object *objects, int objects_count,
                     const TypeStatistics *stats,
                     const Date *current_date,
                     const char *report_filename);

#endif
