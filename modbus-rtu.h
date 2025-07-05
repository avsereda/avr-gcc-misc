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

#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "gpio.h"
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MODBUS_RTU_PARAMS
#define MODBUS_RTU_PARAMS "uart=UART1:9600@8N1,de_port=PORTL:0"
#endif

/* Modbus functions: */
#define MODBUS_FUNC_READ_COILS                  0x01
#define MODBUS_FUNC_READ_DISCRETE_INPUTS        0x02
#define MODBUS_FUNC_READ_HOLDING_REGISTERS      0x03
#define MODBUS_FUNC_READ_INPUT_REGISTERS        0x04
#define MODBUS_FUNC_WRITE_SINGLE_COIL           0x05
#define MODBUS_FUNC_WRITE_SINGLE_REGISTER       0x06
#define MODBUS_FUNC_WRITE_MULTIPLE_COILS        0x0f
#define MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS    0x10


/* Modbus exceptions */
#define MODBUS_EXCEPT_ILLEGAL_FUNC              0x01
#define MODBUS_EXCEPT_ILLEGAL_DATA_ADDR         0x02
#define MODBUS_EXCEPT_ILLEGAL_DATA_VALUE        0x03
#define MODBUS_EXCEPT_SLAVE_DEV_FAILURE         0x04
#define MODBUS_EXCEPT_ACKNOWLEDGE               0x05
#define MODBUS_EXCEPT_SLAVE_DEV_BUSY            0x06
#define MODBUS_EXCEPT_NEGATIVE_ACKNOWLEDGE      0x07
#define MODBUS_EXCEPT_MEMORY_PARITY_ERROR       0x08

#define MODBUS_RESP_DATA_SIZE 32

enum modbus_result {
        MODBUS_RESULT_OK = 0,
        MODBUS_RESULT_INCOMPLETE,
        MODBUS_RESULT_CRC_ERROR,
        MODBUS_RESULT_NOT_ENOUGH_MEMORY_ERROR
};

struct modbus_req {
        uint8_t slave_addr;
        uint8_t func_code;

        const void *data;
        size_t data_size;

        uint16_t quantity;
        uint16_t crc;
};

struct modbus_resp {
        uint8_t slave_addr;
        uint8_t func_code;
        uint8_t except_code;

        uint8_t data[MODBUS_RESP_DATA_SIZE];
        size_t data_size;

        uint16_t crc;
};

struct modbus_rtu {
        struct uart uart;
        struct gpio enable_port;
};

struct modbus_rtu_async {
        int state;

        uint8_t tmp[3];
        struct mem_chunk chunk;

        struct modbus_resp resp;
        enum modbus_result result;

        bool (*recv)(struct modbus_rtu *, struct modbus_rtu_async *);
};

void modbus_req_clear(struct modbus_req *req);
void modbus_resp_clear(struct modbus_resp *resp);
bool modbus_resp_is_exception(struct modbus_resp *resp);
void modbus_rtu_async_init(struct modbus_rtu_async *async);
bool modbus_rtu_async_is_completed(struct modbus_rtu_async *async);
struct modbus_rtu *modbus_rtu_get_instance(void);
bool modbus_rtu_setup(struct modbus_rtu *rtu, const char *params);
bool modbus_rtu_setup_P(struct modbus_rtu *rtu, const char *params);
void modbus_rtu_send_sync(struct modbus_rtu *rtu, struct modbus_req *req);
enum modbus_result modbus_rtu_recv_sync(struct modbus_rtu *rtu, struct modbus_resp *resp);
enum modbus_result modbus_rtu_recv_async(struct modbus_rtu *rtu, struct modbus_rtu_async *async);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RTU_H */
