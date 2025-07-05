#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "modbus-rtu.h"

enum {
        ASYNC_RECV_INIT,
        ASYNC_RECV_HEADER,
        ASYNC_RECV_DATA,
        ASYNC_RECV_CRC,
        ASYNC_RECV_COMPLETED
};

#define UINT16_HI(u16)  ((((uint16_t) (u16)) >> 8) & 0xff)
#define UINT16_LOW(u16) (((uint16_t) (u16)) & 0xff)

/* Initial value of CRC16 register from MODBUS RTU standart */
#define CRC16_REG_INITIALIZER 0xffffu

#define TMP_SIZE 60

void modbus_req_clear(struct modbus_req *req)
{
        memset(req, 0, sizeof(struct modbus_req));
}

void modbus_resp_clear(struct modbus_resp *resp)
{
        memset(resp, 0, sizeof(struct modbus_resp));
}

bool modbus_resp_is_exception(struct modbus_resp *resp)
{
        /*
         * If MSB of function code is set, we got exception response
         */

        return (resp->func_code & 0x80) != 0u;
}

static bool async_recv_impl(struct modbus_rtu *rtu, struct modbus_rtu_async *async)
{
        enum uart_result res = 0;


        res = uart_read_chunk(&rtu->uart, &async->chunk, UART_FLAG_NONBLOCK);
        if (res == UART_RESULT_OK)
                return true;


        return false;
}

static inline enum modbus_result async_recv_transit(struct modbus_rtu_async *async, int next_state);

void modbus_rtu_async_init(struct modbus_rtu_async *async)
{
        modbus_resp_clear(&async->resp);

        memset(async->tmp, 0, sizeof(async->tmp));
        memset(&async->chunk, 0, sizeof(struct mem_chunk));

        async->state = ASYNC_RECV_INIT;
        async->result = MODBUS_RESULT_INCOMPLETE;
        async->recv = async_recv_impl;

        async_recv_transit(async, ASYNC_RECV_HEADER);
}

bool modbus_rtu_async_is_completed(struct modbus_rtu_async *async)
{
        return async->state == ASYNC_RECV_COMPLETED;
}

static inline bool setup_impl(struct modbus_rtu *rtu, char *params)
{
        struct uart *uart = NULL;
        struct gpio *enable_port = NULL;
        char *save_ptr = NULL;
        char *s = NULL;
        char *token = NULL;
        char *param_value = NULL;


        uart = &rtu->uart;
        enable_port = &rtu->enable_port;


        s = params;
        while ((token = strtok_r(s, ",", &save_ptr)) != NULL) {
                s = NULL;

                /* Extract parameter */
                if ((param_value = strpbrk(token, "=")) != NULL)
                        param_value++;


                if (strstr_P(token, PSTR("uart=")) != NULL) {
                        if (!uart_setup(uart, param_value))
                                return false;
                } else if (strstr_P(token, PSTR("de_port=")) != NULL) {
                        if (!gpio_init(enable_port, param_value))
                                return false;
                } else
                        return false;

        }


        return true;
}

bool modbus_rtu_setup(struct modbus_rtu *rtu, const char *params)
{
        char tmp[TMP_SIZE] = {0, };

        strncpy(tmp, params, TMP_SIZE - 1);
        return setup_impl(rtu, tmp);
}

bool modbus_rtu_setup_P(struct modbus_rtu *rtu, const char *params)
{
        char tmp[TMP_SIZE] = {0, };

        strncpy_P(tmp, params, TMP_SIZE - 1);
        return setup_impl(rtu, tmp);
}

struct modbus_rtu *modbus_rtu_get_instance(void)
{
        static struct modbus_rtu rtu;
        static struct modbus_rtu *instance = NULL;


        if (instance == NULL) {
                if (modbus_rtu_setup_P(&rtu, PSTR(MODBUS_RTU_PARAMS))) {
                        instance = &rtu;

                } else {
                        /* FIXME: Log about BUG here! */
                        printf_P(PSTR("// Failed to configure ModBus RTU subsystem.\n"));
                }
        }


        return instance;
}

static inline void crc16_byte(uint16_t *crc_reg, uint8_t byte)
{
        size_t i = 0u;


        *crc_reg ^= byte;

        for (; i < 8u; ++i) {
                if ((*crc_reg & 1u) != 0u) {
                        *crc_reg >>= 1;
                        *crc_reg ^= 0xa001u;
                } else
                        *crc_reg >>= 1;
        }
}

static inline void req_calc_crc(struct modbus_req *req)
{
        size_t i = 0u;
        const uint8_t *bytes = NULL;
        uint16_t crc_reg = CRC16_REG_INITIALIZER;


        crc16_byte(&crc_reg, req->slave_addr);
        crc16_byte(&crc_reg, req->func_code);

        if (req->data != NULL) {
                bytes = (const uint8_t *) req->data;
                for (; i < req->data_size; ++i)
                        crc16_byte(&crc_reg, bytes[i]);
        }

        crc16_byte(&crc_reg, UINT16_HI(req->quantity));
        crc16_byte(&crc_reg, UINT16_LOW(req->quantity));

        req->crc = crc_reg;
}

static inline void set_driver_enable(struct modbus_rtu *rtu, bool enable)
{
        /*
         * NOTE: We use external pullup resistor on DE line,
         *       so we set line HIGH by changing direction to input
         */

        if (enable)
                gpio_set_direction(&rtu->enable_port, GPIO_DIRECTION_INPUT);
        else
                gpio_write(&rtu->enable_port, GPIO_STATE_LOW);
}

static inline void send_byte(struct modbus_rtu *rtu, uint8_t byte)
{
        uart_write_byte(&rtu->uart, byte, UART_FLAG_SYNC_TXC);
}

static inline void send(struct modbus_rtu *rtu, const uint8_t *data, size_t size)
{
        struct mem_chunk chunk;


        if (data != NULL) {
                mem_chunk_set(&chunk, (void *) data, size);
                uart_write_chunk(&rtu->uart, &chunk, UART_FLAG_SYNC_TXC);
        }
}

void modbus_rtu_send_sync(struct modbus_rtu *rtu, struct modbus_req *req)
{
        req_calc_crc(req);

        set_driver_enable(rtu, true);

        send_byte(rtu, req->slave_addr);
        send_byte(rtu, req->func_code);
        send(rtu, req->data, req->data_size);
        send_byte(rtu, UINT16_HI(req->quantity));
        send_byte(rtu, UINT16_LOW(req->quantity));
        send_byte(rtu, UINT16_LOW(req->crc));
        send_byte(rtu, UINT16_HI(req->crc));

        set_driver_enable(rtu, false);
}

static inline void recv_byte(struct modbus_rtu *rtu, uint8_t *ptr)
{
        uart_read_byte(&rtu->uart, ptr, 0);
}

static inline void recv(struct modbus_rtu *rtu, uint8_t *buf, size_t size)
{
        struct mem_chunk chunk;


        if (buf != NULL) {
                mem_chunk_set(&chunk, buf, size);
                uart_read_chunk(&rtu->uart, &chunk, 0);
        }
}

static inline bool resp_check_crc(struct modbus_resp *resp)
{
        uint16_t crc_reg = CRC16_REG_INITIALIZER;
        size_t i = 0u;
        uint8_t size = 0u;


        crc16_byte(&crc_reg, resp->slave_addr);
        crc16_byte(&crc_reg, resp->func_code);

        if (modbus_resp_is_exception(resp)) {
                crc16_byte(&crc_reg, resp->except_code);

        } else {
                size = (uint8_t) resp->data_size;
                crc16_byte(&crc_reg, size);
                for (; i < resp->data_size && i < (size_t) MODBUS_RESP_DATA_SIZE; ++i)
                        crc16_byte(&crc_reg, resp->data[i]);
        }


        return crc_reg == resp->crc;
}

enum modbus_result modbus_rtu_recv_sync(struct modbus_rtu *rtu, struct modbus_resp *resp)
{
        uint8_t tmp = 0u;


        recv_byte(rtu, &resp->slave_addr);
        recv_byte(rtu, &resp->func_code);

        if (!modbus_resp_is_exception(resp)) {
                recv_byte(rtu, &tmp);
                if ((size_t) tmp >= MODBUS_RESP_DATA_SIZE)
                        return MODBUS_RESULT_NOT_ENOUGH_MEMORY_ERROR;

                resp->data_size = tmp;
                recv(rtu, resp->data, resp->data_size);
        } else
                recv_byte(rtu, &resp->except_code);


        recv_byte(rtu, &tmp);
        resp->crc |= (uint16_t) tmp;
        recv_byte(rtu, &tmp);
        resp->crc |= (uint16_t) tmp << 8;

        if (!resp_check_crc(resp))
                return MODBUS_RESULT_CRC_ERROR;


        return MODBUS_RESULT_OK;
}

static inline enum modbus_result async_recv_transit(struct modbus_rtu_async *async, int next_state)
{
        struct mem_chunk *chunk = NULL;


        if (async->state != next_state) {
                chunk = &async->chunk;

                if (next_state == ASYNC_RECV_HEADER)
                        mem_chunk_set(chunk, async->tmp, 3u);
                else if (next_state == ASYNC_RECV_DATA)
                        mem_chunk_set(chunk, async->resp.data, async->resp.data_size);
                else if (next_state == ASYNC_RECV_CRC)
                        mem_chunk_set(chunk, async->tmp, 2u);
        }


        async->state = next_state;
        return async->result;
}

static inline enum modbus_result async_recv_complete(struct modbus_rtu_async *async,
                                                     enum modbus_result result)
{
        async->state = ASYNC_RECV_COMPLETED;
        async->result = result;

        return async->result;
}

static inline enum modbus_result recv_async_header(struct modbus_rtu_async *async)
{
        struct modbus_resp *resp = NULL;


        resp = &async->resp;

        resp->slave_addr = async->tmp[0];
        resp->func_code = async->tmp[1];

        if (!modbus_resp_is_exception(resp)) {
                if ((size_t) async->tmp[2] >= MODBUS_RESP_DATA_SIZE) {
                        return async_recv_complete(async,
                                                   MODBUS_RESULT_NOT_ENOUGH_MEMORY_ERROR);
                }

                resp->data_size = async->tmp[2];
                return async_recv_transit(async, ASYNC_RECV_DATA);
        }


        resp->except_code = async->tmp[2];
        return async_recv_transit(async, ASYNC_RECV_CRC);
}

static inline enum modbus_result recv_async_crc(struct modbus_rtu_async *async)
{
        struct modbus_resp *resp = NULL;


        resp = &async->resp;

        resp->crc = 0u;
        resp->crc |= (uint16_t) async->tmp[0];
        resp->crc |= (uint16_t) async->tmp[1] << 8;

        if (resp_check_crc(resp))
                return async_recv_complete(async, MODBUS_RESULT_OK);


        return async_recv_complete(async, MODBUS_RESULT_CRC_ERROR);
}

enum modbus_result modbus_rtu_recv_async(struct modbus_rtu *rtu, struct modbus_rtu_async *async)
{
        if (modbus_rtu_async_is_completed(async))
                return async->result;


        if (async->recv(rtu, async)) {
                if (async->state == ASYNC_RECV_HEADER)
                        return recv_async_header(async);
                else if (async->state == ASYNC_RECV_DATA)
                        return async_recv_transit(async, ASYNC_RECV_CRC);
                else if (async->state == ASYNC_RECV_CRC)
                        return recv_async_crc(async);
        }


        return async->result;
}
