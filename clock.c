#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "clock.h"

/*
 * NOTE: On 16Mhz clock with prescaler 64, Timer/Counter reaches this value at 1ms interval
 */
#define CLOCK_OCR_VALUE 249u

static struct clock_time sys_time = {0, };

void clock_setup(void)
{
        static bool is_ready = false;


        if (!is_ready) {
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                        TCNT0 = (uint8_t) 0u;
                        OCR0A = (uint8_t) CLOCK_OCR_VALUE;
                        TIMSK0 = (uint8_t)(_BV(OCIE0A));           /* Enable interrupts on compare (Match A) */
                        TCCR0B = (uint8_t)(_BV(CS01) | _BV(CS00)); /* Start Timer/Counter (Prescaler is 64) */
                }

                is_ready = true;
        }
}

void clock_get_time(struct clock_time *ct)
{
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                memcpy(ct, &sys_time, sizeof(struct clock_time));

        }
}

double clock_diff(struct clock_time *x, struct clock_time *y)
{
        double x_msec = NAN;
        double y_msec = NAN;


        x_msec = CLOCK_TIME_TO_DOUBLE(x);
        y_msec = CLOCK_TIME_TO_DOUBLE(y);


        return MAX(x_msec, y_msec) - MIN(x_msec, y_msec);
}

int clock_cmp(struct clock_time *x, struct clock_time *y)
{
        if (x->sec > y->sec)
                return -1;
        else if (x->sec < y->sec)
                return 1;

        if (x->msec > y->msec)
                return -1;
        else if (y->msec < y->msec)
                return 1;


        return 0;
}

ISR(TIMER0_COMPA_vect)
{
        if (sys_time.msec == 999ul) {
                sys_time.msec = 0ul;
                sys_time.sec++;

        } else
                sys_time.msec++;
}
