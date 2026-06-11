#include "dates.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int month, int year) {
    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month - 1];
}

int parse_date(const char *str, Date *out) {
    if (sscanf(str, "%d.%d.%d", &out->day, &out->month, &out->year) != 3) {
        return 0;
    }
    return is_date_valid(out);
}

int is_date_valid(const Date *d) {
    if (d->year < 1900 || d->year > 2100) return 0;
    if (d->month < 1 || d->month > 12) return 0;
    if (d->day < 1 || d->day > days_in_month(d->month, d->year)) return 0;
    return 1;
}

int compare_dates(const Date *d1, const Date *d2) {
    if (d1->year != d2->year) return (d1->year > d2->year) ? 1 : -1;
    if (d1->month != d2->month) return (d1->month > d2->month) ? 1 : -1;
    if (d1->day != d2->day) return (d1->day > d2->day) ? 1 : -1;
    return 0;
}

int days_between(const Date *from, const Date *to) {
    Date temp = *from;
    int days = 0;
    
    while (compare_dates(&temp, to) < 0) {
        temp.day++;
        if (temp.day > days_in_month(temp.month, temp.year)) {
            temp.day = 1;
            temp.month++;
            if (temp.month > 12) {
                temp.month = 1;
                temp.year++;
            }
        }
        days++;
    }
    return days;
}

void print_date(const Date *d, char *buffer, size_t buf_size) {
    snprintf(buffer, buf_size, "%02d.%02d.%04d", d->day, d->month, d->year);
}
