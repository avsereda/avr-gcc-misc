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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <math.h>

#include "gpio.h"
#include "uart.h"
#include "panic.h"
#include "timer.h"

#ifndef UART_DEV_PARAMS
#define UART_DEV_PARAMS        "UART0:115200@8N1"
#endif


int main(void)
{
        struct uart uart;
        struct timer poll_timer;

        wdt_disable();
        cli();

        clock_setup();
        if (!uart_setup_P(&uart, PSTR(UART_DEV_PARAMS))) {
                panic();

        }

        uart_bind_to_cstdin(&uart);
        uart_bind_to_cstdout(&uart);
        uart_bind_to_cstderr(&uart);

        sei();

        timer_clear(&poll_timer);
        timer_set_msecs(&poll_timer, 3000u);

        for (;;) {

        }


        return 0;
}
