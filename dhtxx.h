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

#ifndef DHTXX_H
#define DHTXX_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DHTXX_STRICT_TIMINGS 1

enum dhtxx_result {
        DHTXX_RESULT_SUCCESS = 0,
        DHTXX_RESULT_NO_RESPONSE,
        DHTXX_RESULT_BAD_CHECKSUM
};

struct dhtxx {
        struct gpio *port;

        int16_t raw_temperature;
        int16_t raw_humidity;
};

void dhtxx_init(struct dhtxx *dht, struct gpio *dht_port);
enum dhtxx_result dhtxx_poll(struct dhtxx *dht);
double dhtxx_get_temperature(struct dhtxx *dht);
double dhtxx_get_humidity(struct dhtxx *dht);
bool dhtxx_data_json_stringify(struct dhtxx *dht, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* DHTXX_H */
