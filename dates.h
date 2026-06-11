#ifndef DATES_H
#define DATES_H

#include <stddef.h>

typedef struct {
    int day;
    int month;
    int year;
} Date;

int parse_date(const char *str, Date *out);
int is_date_valid(const Date *d);
int compare_dates(const Date *d1, const Date *d2);
int days_between(const Date *from, const Date *to);
void print_date(const Date *d, char *buffer, size_t buf_size);

#endif
