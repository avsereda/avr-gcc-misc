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

#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

#include "fifo-buffer.h"
#include "mem-chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_HW_TX_FIFO_SIZE 16
#define UART_HW_RX_FIFO_SIZE 16

enum {
        UART_POLL_IN = 1,
        UART_POLL_OUT = 2
};

enum {
        UART_FLAG_NONBLOCK = 1,
        UART_FLAG_TEXT_MODE = 2,
        UART_FLAG_SYNC_TXC = 4
};

enum uart_result {
        UART_RESULT_OK = 0,
        UART_RESULT_WILL_BLOCK
};

struct uart_hw_registers {
        volatile uint8_t *ucsrxa_addr;
        volatile uint8_t *ucsrxb_addr;
        volatile uint8_t *ucsrxc_addr;
        volatile uint8_t *ubrrxh_addr;
        volatile uint8_t *ubrrxl_addr;
        volatile uint8_t *udrx_addr;
};

struct uart_hw {
        uint8_t _tx_fifo_buf[UART_HW_TX_FIFO_SIZE];
        uint8_t _rx_fifo_buf[UART_HW_RX_FIFO_SIZE];

        struct fifo_buffer tx_fifo;
        struct fifo_buffer rx_fifo;

        struct uart_hw_registers reg;
};

struct uart {
        struct uart_hw *hw;

        bool (*setup)(struct uart_hw *, unsigned long, unsigned, char, unsigned);
        void (*intr_tx_enable)(struct uart_hw *);
        void (*intr_rx_enable)(struct uart_hw *);
        void (*intr_tx_disable)(struct uart_hw *);
        void (*intr_rx_disable)(struct uart_hw *);
        bool (*is_udr_empty)(struct uart_hw *);
        bool (*is_tx_complete)(struct uart_hw *);
        void (*clear_txc)(struct uart_hw *);
};

bool uart_setup(struct uart *dev, const char *params);
bool uart_setup_P(struct uart *dev, const char *params);
int uart_poll(struct uart *dev, int event_mask);
void uart_flush(struct uart *dev);
enum uart_result uart_read_byte(struct uart *dev, uint8_t *store, int flags);
enum uart_result uart_write_byte(struct uart *dev, uint8_t byte, int flags);
enum uart_result uart_read_chunk(struct uart *dev, struct mem_chunk *chunk, int flags);
enum uart_result uart_write_chunk(struct uart *dev, struct mem_chunk *chunk, int flags);
void uart_bind_to_cstdin(struct uart *dev);
void uart_bind_to_cstdout(struct uart *dev);
void uart_bind_to_cstderr(struct uart *dev);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
