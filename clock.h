/*
 * Copyright (c) 2018 Sereda Anton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX(a, b)                               \
        ({                                      \
                __typeof__((a)) _a = (a);       \
                __typeof__((b)) _b = (b);       \
                (_a > _b) ? _a: _b;             \
        })

#define MIN(a, b)                               \
        ({                                      \
                __typeof__((a)) _a = (a);       \
                __typeof__((b)) _b = (b);       \
                (_a < _b) ? _a: _b;             \
        })

#define CLOCK_TIME_TO_DOUBLE(ct)                \
        ((double) ((1000.0 * (ct)->sec) + (ct)->msec))


struct clock_time {
        unsigned long sec;
        unsigned long msec;
};

void clock_setup(void);
void clock_get_time(struct clock_time *ct);
double clock_diff(struct clock_time *x, struct clock_time *y);
int clock_cmp(struct clock_time *x, struct clock_time *y);

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_H */
