#include <math.h>
#include <string.h>

#include "timer.h"

void timer_clear(struct timer *t)
{
        memset(t, 0, sizeof(struct timer));
}

void timer_set(struct timer *t, struct clock_time *interval)
{
        timer_clear(t);
        timer_reset(t);

        if (interval != NULL) {
                memcpy(&t->interval, interval, sizeof(struct clock_time));

        }
}

void timer_set_msecs(struct timer *t, unsigned long msec_interval)
{
        timer_clear(t);
        timer_reset(t);

        t->interval.sec = (unsigned long) (msec_interval / 1000ul);
        t->interval.msec = (unsigned long) (msec_interval % 1000);
}

void timer_reset(struct timer *t)
{
        clock_get_time(&t->start);
}

bool timer_expired(struct timer *t)
{
        return timer_remaining(t) < 0.0;
}

double timer_remaining(struct timer *t)
{
        struct clock_time ct_buf = {0, };
        double now = NAN;
        double interval = NAN;
        double start = NAN;


        clock_get_time(&ct_buf);

        start = CLOCK_TIME_TO_DOUBLE(&t->start);
        interval = CLOCK_TIME_TO_DOUBLE(&t->interval);
        now = CLOCK_TIME_TO_DOUBLE(&ct_buf);


        return (start + interval) - now;
}
