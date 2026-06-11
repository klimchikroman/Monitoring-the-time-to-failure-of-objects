#include "calc.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

double calculate_operating_time(const Object *obj, const Date *current_date, double daily_hours) {
    if (obj->status == -1) {
        return 0.0;
    }
    
    if (compare_dates(&obj->start_date, current_date) > 0) {
        return 0.0;
    }
    
    Date effective_end;
    
    if (obj->status == 0) {
        effective_end = *current_date;
    } else if (obj->status == 1) {
        effective_end = obj->end_date;
    } else { 
        if (compare_dates(&obj->end_date, current_date) > 0) {
            effective_end = *current_date;
        } else {
            effective_end = obj->end_date;
        }
    }
    
    if (compare_dates(&obj->start_date, &effective_end) > 0) {
        return 0.0;
    }
    
    int days = days_between(&obj->start_date, &effective_end);
    return days * daily_hours;
}

void calculate_all_operating_times(Object *objects, int count, const Date *current_date,
                                    const Type *types, int types_count) {
    for (int i = 0; i < count; i++) {
        double daily_hours = 0.0;
        for (int j = 0; j < types_count; j++) {
            if (types[j].id == objects[i].type_id) {
                daily_hours = types[j].daily_hours;
                break;
            }
        }
        objects[i].operating_time = calculate_operating_time(&objects[i], current_date, daily_hours);
    }
}

int compute_type_statistics(int type_id, const Object *objects, int objects_count,
                            const Type *types, int types_count,
                            const Date *current_date, TypeStatistics *stat) {
    stat->type_id = type_id;
    stat->total = 0;
    stat->operational = 0;
    stat->removed = 0;
    stat->failed = 0;
    stat->defective = 0;
    stat->defect_rate = 0.0;
    stat->mttf = 0.0;
    stat->mttf_valid = 0;
    stat->lambda = 0.0;
    stat->lambda_valid = 0;
    stat->predicted_failures_3m = 0.0;
    stat->predicted_failures_6m = 0.0;
    stat->predicted_failures_12m = 0.0;
    stat->recommended_tbo = 0.0;
    stat->tbo_probability = 0.0;
    
    double design_lifetime = 0.0;
    double daily_hours = 0.0;
    for (int i = 0; i < types_count; i++) {
        if (types[i].id == type_id) {
            design_lifetime = types[i].design_lifetime;
            daily_hours = types[i].daily_hours;
            break;
        }
    }
    
    double total_operating_time_all = 0.0;  
    int has_any_object = 0; 
    
    for (int i = 0; i < objects_count; i++) {
        if (objects[i].type_id != type_id) continue;
        
        has_any_object = 1;  
        
        if (objects[i].status == -1) {
            continue;
        }
        
        int is_operational = (compare_dates(&objects[i].start_date, current_date) <= 0);
        
        if (!is_operational) {
            continue;
        }
        
        int actual_status = objects[i].status;
        if (actual_status == 2 && compare_dates(&objects[i].end_date, current_date) > 0) {
            actual_status = 0; 
        }
        
        double op_time = objects[i].operating_time;
        
        total_operating_time_all += op_time;
        
        int include_in_total = 0;
        
        switch (actual_status) {
            case 2: 
                include_in_total = 1;
                stat->failed++;
                if (op_time < design_lifetime) {
                    stat->defective++;
                }
                break;
                
            case 0:  
                if (op_time >= design_lifetime) {
                    include_in_total = 1;  
                }
                stat->operational++;
                break;
                
            case 1: 
                if (op_time >= design_lifetime) {
                    include_in_total = 1;  
                }
                stat->removed++;
                break;
        }
        
        if (include_in_total) {
            stat->total++;
        }
    }
    
    if (!has_any_object) return 0;

    if (stat->total > 0) {
        stat->defect_rate = (double)stat->defective / stat->total * 100.0;
    } else {
        stat->defect_rate = 0.0;
    }

    if (total_operating_time_all > 0 && stat->failed > 0) {
        stat->lambda = (double)stat->failed / total_operating_time_all;
        stat->lambda_valid = 1;
        stat->mttf = 1.0 / stat->lambda;
        stat->mttf_valid = 1;
    } else if (total_operating_time_all > 0 && stat->failed == 0) {
        stat->lambda = 0.0;
        stat->lambda_valid = 1;
        stat->mttf = 0.0;
        stat->mttf_valid = 0;  
    }
    
    return 1;
}

void compute_exponential_statistics(TypeStatistics *stat, const Type *type) {
    if (!stat->lambda_valid) {
        return;
    }
    
    int days_per_month = 30;
    predict_failures(stat, type, days_per_month);
    
    double max_failure_prob = 0.05;  // 5%
    calculate_recommended_tbo(stat, type, max_failure_prob);
}

void predict_failures(TypeStatistics *stat, const Type *type, int days_per_month) {
    if (!stat->lambda_valid || stat->lambda == 0.0) {
        stat->predicted_failures_3m = 0.0;
        stat->predicted_failures_6m = 0.0;
        stat->predicted_failures_12m = 0.0;
        return;
    }
    
    int operating_count = stat->operational;
    
    if (operating_count == 0) {
        stat->predicted_failures_3m = 0.0;
        stat->predicted_failures_6m = 0.0;
        stat->predicted_failures_12m = 0.0;
        return;
    }
    
    double hours_3m = 3.0 * days_per_month * type->daily_hours;
    double hours_6m = 6.0 * days_per_month * type->daily_hours;
    double hours_12m = 12.0 * days_per_month * type->daily_hours;
    
    stat->predicted_failures_3m = operating_count * (1.0 - exp(-stat->lambda * hours_3m));
    stat->predicted_failures_6m = operating_count * (1.0 - exp(-stat->lambda * hours_6m));
    stat->predicted_failures_12m = operating_count * (1.0 - exp(-stat->lambda * hours_12m));
}

void calculate_recommended_tbo(TypeStatistics *stat, const Type *type, double max_failure_probability) {
    if (!stat->lambda_valid || stat->lambda == 0.0) {
        stat->recommended_tbo = type->design_lifetime;
        stat->tbo_probability = 0.0;
        return;
    }
    
    double t = -log(1.0 - max_failure_probability) / stat->lambda;
    
    if (t > type->design_lifetime) {
        t = type->design_lifetime;
    }
    if (t < 1.0) {
        t = 1.0;
    }
    
    stat->recommended_tbo = t;
    stat->tbo_probability = max_failure_probability * 100.0;
}
