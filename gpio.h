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

#ifndef GPIO_H
#define GPIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum gpio_state {
        GPIO_STATE_LOW = 0,
        GPIO_STATE_HIGH
};

enum gpio_direction {
        GPIO_DIRECTION_INPUT = 0,
        GPIO_DIRECTION_OUTPUT
};

struct gpio_addr_table {
        const volatile uint8_t *pin_addr;
        volatile uint8_t *port_addr;
        volatile uint8_t *ddr_addr;
};

struct gpio {
        const struct gpio_addr_table *addr_table;
        enum gpio_direction direction;
        unsigned bit;
};

static inline enum gpio_direction gpio_get_direction(struct gpio *port)
{
        return port->direction;
}

static inline bool gpio_is_usable(struct gpio *port)
{
        return port->addr_table != NULL;
}

bool gpio_init(struct gpio *port, const char *params);
bool gpio_init_P(struct gpio *port, const char *params);
void gpio_set_direction(struct gpio *port, enum gpio_direction dir);
enum gpio_state gpio_read(struct gpio *port);
void gpio_write(struct gpio *port, enum gpio_state state);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
