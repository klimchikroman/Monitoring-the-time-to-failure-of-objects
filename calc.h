#ifndef CALC_H
#define CALC_H

#include "dates.h"
#include "load.h"
#include <math.h>

typedef struct {
    int type_id;
    int total;
    int operational;
    int removed;
    int failed;
    int defective;
    double defect_rate;
    double mttf;              
    int mttf_valid;
    double lambda;             
    int lambda_valid;
    double predicted_failures_3m;  
    double predicted_failures_6m;  
    double predicted_failures_12m; 
    double recommended_tbo;         
    double tbo_probability;         
} TypeStatistics;

double calculate_operating_time(const Object *obj, const Date *current_date, double daily_hours);
void calculate_all_operating_times(Object *objects, int count, const Date *current_date,
                                    const Type *types, int types_count);
int compute_type_statistics(int type_id, const Object *objects, int objects_count,
                            const Type *types, int types_count, 
                            const Date *current_date, TypeStatistics *stat);

void compute_exponential_statistics(TypeStatistics *stat, const Type *type);
void predict_failures(TypeStatistics *stat, const Type *type, int days_per_month);
void calculate_recommended_tbo(TypeStatistics *stat, const Type *type, double max_failure_probability);

#endif
