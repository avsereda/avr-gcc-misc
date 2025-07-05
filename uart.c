#include <string.h>
#include <stdio.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/cpufunc.h>

/*
 * TODO: Framing error detection (Parity checking)
 */

#include "uart.h"

#define FLAG_IS_SET(mask, flag) (((mask) & (flag)) == (flag))

#define IDLE_LOOP_IF(value)     \
        while ((value)) {       \
                _NOP();         \
                _NOP();         \
                _NOP();         \
                _NOP();         \
                _NOP();         \
                _NOP();         \
        }

#define IDLE_LOOP_IF_NOT(value) IDLE_LOOP_IF(!(value))

enum {
        UART0 = 0,
        UART1,
        UART2,
        UART3,

        N_UART_DEVICES
};

static struct uart *cstdin_dev = NULL;
static struct uart *cstdout_dev = NULL;
static struct uart *cstderr_dev = NULL;

static struct uart_hw hw_devices[N_UART_DEVICES] = {
        { .reg = {&UCSR0A, &UCSR0B, &UCSR0C, &UBRR0H, &UBRR0L, &UDR0} },
        { .reg = {&UCSR1A, &UCSR1B, &UCSR1C, &UBRR1H, &UBRR1L, &UDR1} },
        { .reg = {&UCSR2A, &UCSR2B, &UCSR2C, &UBRR2H, &UBRR2L, &UDR2} },
        { .reg = {&UCSR3A, &UCSR3B, &UCSR3C, &UBRR3H, &UBRR3L, &UDR3} }
};

static bool uart_hw_setup(struct uart_hw *hw,
                          unsigned long baud_rate,
                          unsigned frame_size,
                          char parity_sign,
                          unsigned n_stop_bits)
{
        struct uart_hw_registers *reg = NULL;
        uint16_t ubrr_value = 0u;


        reg = &hw->reg;

        fifo_buffer_init(&hw->rx_fifo, hw->_rx_fifo_buf, UART_HW_RX_FIFO_SIZE);
        fifo_buffer_init(&hw->tx_fifo, hw->_tx_fifo_buf, UART_HW_TX_FIFO_SIZE);

        /* Clear control registers */
        *( reg->ucsrxa_addr ) = (uint8_t) 0u;
        *( reg->ucsrxb_addr ) = (uint8_t) 0u;
        *( reg->ucsrxc_addr ) = (uint8_t) 0u;

        /* Calculate and set baud rate */
        ubrr_value = (uint16_t) round(((double) F_CPU / (16.0 * (double) baud_rate)) - 1.0);

        *( reg->ubrrxl_addr ) = (uint8_t)(ubrr_value & 0xff);
        *( reg->ubrrxh_addr ) = (uint8_t)(ubrr_value >> 8);

        /* Set number of stop bits */
        /* NOTE: By default we use one stop bit */
        if (n_stop_bits == 2u) {
                *( reg->ucsrxc_addr ) |= (uint8_t)(_BV(USBS0));
        }

        /* Set frame format */
        if (frame_size == 8u) {
                *( reg->ucsrxc_addr ) |= (uint8_t)(_BV(UCSZ01) | _BV(UCSZ00));
        } else if (frame_size == 7u) {
                *( reg->ucsrxc_addr ) |= (uint8_t)(_BV(UCSZ01));
        }

        /* Enable transmitter / receiver */
        *( reg->ucsrxb_addr ) |= (uint8_t)(_BV(TXEN0) | _BV(RXEN0));


        return true;
}

static void uart_hw_intr_tx_enable(struct uart_hw *hw)
{
        *( hw->reg.ucsrxb_addr ) |= (uint8_t)(_BV(UDRIE0));
}

static void uart_hw_intr_rx_enable(struct uart_hw *hw)
{
        *( hw->reg.ucsrxb_addr ) |= (uint8_t)(_BV(RXCIE0));
}

static void uart_hw_intr_tx_disable(struct uart_hw *hw)
{
        *( hw->reg.ucsrxb_addr ) &= (uint8_t) ~(_BV(UDRIE0));
}

static void uart_hw_intr_rx_disable(struct uart_hw *hw)
{
        *( hw->reg.ucsrxb_addr ) &= (uint8_t) ~(_BV(RXCIE0));
}

static bool uart_hw_is_udr_empty(struct uart_hw *hw)
{
        return (bool) (*( hw->reg.ucsrxa_addr ) & _BV(UDRIE0));
}

static bool uart_hw_is_tx_complete(struct uart_hw *hw)
{
        return (bool) (*( hw->reg.ucsrxa_addr ) & _BV(TXC0));
}

static void uart_hw_clear_txc(struct uart_hw *hw)
{
        /* NOTE: TXC bit may be cleared by setting one to its position */

        *( hw->reg.ucsrxa_addr ) |= (uint8_t) (_BV(TXC0));
}

static inline bool is_parity_sign(int parity_sign)
{
        return parity_sign == 'N';
}

static inline bool is_supported_frame_size(unsigned frame_size)
{
        /* FIXME: Other frame formats is irrelevant for me! */

        return frame_size == 8u || frame_size == 7u;
}

static inline bool is_supported_n_stop_bits(unsigned stop_bits)
{
        return stop_bits == 2u || stop_bits == 1u;
}

static inline bool is_supported_baud_rate(unsigned long baud_rate)
{
        static unsigned long const rates[] PROGMEM = {
                50u, 75u, 110u, 134u, 150u, 200u, 300u, 600u,
                1200u, 1800u, 2400u, 4800u, 9600u, 19200u, 38400u,
                57600u, 115200u
        };

        size_t i = 0u;


        for (; i < sizeof(rates) / sizeof(rates[0]); ++i) {
                if (pgm_read_dword_near(&rates[i]) == baud_rate)
                        return true;

        }


        return false;
}

bool uart_setup(struct uart *dev, const char *params)
{
        struct uart_hw *hw = NULL;
        unsigned dev_num = 0u;
        unsigned long baud_rate = 0u;
        unsigned frame_size = 0u;
        char parity_sign = 0;
        unsigned n_stop_bits = 0u;


        memset(dev, 0, sizeof(struct uart));

        if (sscanf_P(params, PSTR("UART%u:%lu@%u%c%u"), &dev_num, &baud_rate, &frame_size,
                     &parity_sign, &n_stop_bits) != 5) {

                return false;
        }

        if (dev_num < N_UART_DEVICES && is_parity_sign(parity_sign)
                        && is_supported_frame_size(frame_size)
                        && is_supported_n_stop_bits(n_stop_bits)
                        && is_supported_baud_rate(baud_rate)) {

                hw = &hw_devices[dev_num];
                dev->hw = hw;

                /* Setup hardware controll functions */
                dev->setup = uart_hw_setup;
                dev->intr_tx_enable = uart_hw_intr_tx_enable;
                dev->intr_rx_enable = uart_hw_intr_rx_enable;
                dev->intr_tx_disable = uart_hw_intr_tx_disable;
                dev->intr_rx_disable = uart_hw_intr_rx_disable;
                dev->is_udr_empty = uart_hw_is_udr_empty;
                dev->is_tx_complete = uart_hw_is_tx_complete;
                dev->clear_txc = uart_hw_clear_txc;

                /* If we try to setup device not at first time... */
                dev->intr_rx_disable(hw);
                dev->intr_tx_disable(hw);

                if (dev->setup(hw, baud_rate, frame_size,
                               parity_sign, n_stop_bits)) {

                        dev->intr_rx_enable(hw);
                        return true;
                }
        }


        return false;
}

bool uart_setup_P(struct uart *dev, const char *params)
{
        char buf[32] = {0, };


        strncpy_P(buf, params, sizeof(buf) - 1);
        return uart_setup(dev, buf);
}

int uart_poll(struct uart *dev, int event_mask)
{
        int revents = 0;
        struct uart_hw *hw = NULL;


        hw = dev->hw;


        if (event_mask == 0)
                return 0;

        if (FLAG_IS_SET(event_mask, UART_POLL_IN)) {
                if (!fifo_buffer_is_empty(&hw->rx_fifo))
                        revents |= UART_POLL_IN;
                else
                        dev->intr_rx_enable(hw);
        }

        if (FLAG_IS_SET(event_mask, UART_POLL_OUT)) {
                if (!fifo_buffer_is_full(&hw->tx_fifo))
                        revents |= UART_POLL_OUT;
                else
                        dev->intr_tx_enable(hw);
        }

        return revents;
}

void uart_flush(struct uart *dev)
{
        struct uart_hw *hw = NULL;


        hw = dev->hw;

        dev->intr_tx_enable(hw);
        IDLE_LOOP_IF_NOT(fifo_buffer_is_empty(&hw->tx_fifo));
}

enum uart_result uart_read_byte(struct uart *dev, uint8_t *store, int flags)
{
        struct uart_hw *hw = NULL;


        hw = dev->hw;

        if (fifo_buffer_is_empty(&hw->rx_fifo)) {
                dev->intr_rx_enable(hw);

                if (FLAG_IS_SET(flags, UART_FLAG_NONBLOCK))
                        return UART_RESULT_WILL_BLOCK;

                /* Wait for data... */
                IDLE_LOOP_IF(fifo_buffer_is_empty(&hw->rx_fifo));
        }

        /* NOTE: Rx FIFO can't be empty here! */
        fifo_buffer_get_byte(&hw->rx_fifo, store);

        /* Translate CR to LF on input */
        if (FLAG_IS_SET(flags, UART_FLAG_TEXT_MODE) && *store == '\r')
                *store = (uint8_t) '\n';


        return UART_RESULT_OK;
}

static inline void write_byte_sync(struct uart *dev, uint8_t byte)
{
        struct uart_hw *hw = NULL;


        hw = dev->hw;

        IDLE_LOOP_IF_NOT(dev->is_udr_empty(hw));

        dev->clear_txc(hw);
        *( hw->reg.udrx_addr ) = byte;

        IDLE_LOOP_IF_NOT(dev->is_tx_complete(hw));
}

enum uart_result uart_write_byte(struct uart *dev, uint8_t byte, int flags)
{
        struct uart_hw *hw = NULL;


        hw = dev->hw;

        if (FLAG_IS_SET(flags, UART_FLAG_SYNC_TXC)) {
                /*
                 * NOTE: if user wants synchronous output, we don't use TX buffer and interrupts...
                 */

                dev->intr_tx_disable(hw);

                if (FLAG_IS_SET(flags, UART_FLAG_TEXT_MODE) && (int) byte == '\n')
                        write_byte_sync(dev, (uint8_t) '\r');

                write_byte_sync(dev, byte);
                dev->intr_tx_enable(hw);
                return UART_RESULT_OK;
        }


        if (fifo_buffer_is_full(&hw->tx_fifo)) {
                if (FLAG_IS_SET(flags, UART_FLAG_NONBLOCK)) {
                        dev->intr_tx_enable(hw);
                        return UART_RESULT_WILL_BLOCK;
                }

                uart_flush(dev);
        }

        if (FLAG_IS_SET(flags, UART_FLAG_TEXT_MODE) && (int) byte == '\n') {
                /* We need at least 2 bytes of free space in the TX buffer or we need to flush it */
                if (fifo_buffer_get_unused_size(&hw->tx_fifo) < 2u
                                && FLAG_IS_SET(flags, UART_FLAG_NONBLOCK)) {

                        dev->intr_tx_enable(hw);
                        return UART_RESULT_WILL_BLOCK;
                }

                uart_flush(dev);
                fifo_buffer_put_byte(&hw->tx_fifo, (uint8_t) '\r');
        }

        fifo_buffer_put_byte(&hw->tx_fifo, byte);

        if (FLAG_IS_SET(flags, UART_FLAG_TEXT_MODE) && (int) byte == '\n') {
                if (FLAG_IS_SET(flags, UART_FLAG_NONBLOCK)) {
                        dev->intr_tx_enable(hw);
                        return UART_RESULT_OK;
                }

                uart_flush(dev);
        }


        return UART_RESULT_OK;
}

enum uart_result uart_read_chunk(struct uart *dev, struct mem_chunk *chunk, int flags)
{
        size_t i = 0u;
        uint8_t *bytes = NULL;
        enum uart_result result = UART_RESULT_OK;


        i = chunk->offset;
        bytes = (uint8_t *) chunk->ptr;

        for (; i < chunk->size; ++i) {
                result = uart_read_byte(dev, &bytes[i], flags);
                if (result != UART_RESULT_OK)
                        break;
        }


        chunk->offset = i;
        return result;
}

enum uart_result uart_write_chunk(struct uart *dev, struct mem_chunk *chunk, int flags)
{
        size_t i = 0u;
        uint8_t *bytes = NULL;
        enum uart_result result = UART_RESULT_OK;


        i = chunk->offset;
        bytes = (uint8_t *) chunk->ptr;

        for (; i < chunk->size; ++i) {
                result = uart_write_byte(dev, bytes[i], flags);
                if (result != UART_RESULT_OK)
                        break;
        }


        chunk->offset = i;

        if (!FLAG_IS_SET(flags, UART_FLAG_SYNC_TXC))
                uart_flush(dev);


        return result;
}

#define DEFINE_PUTC(name)                                                                       \
        static int name##_putc(char ch, FILE *stream)                                           \
        {                                                                                       \
                if (name##_dev != NULL) {                                                       \
                        uart_write_byte(name##_dev, (uint8_t) ch, UART_FLAG_TEXT_MODE);         \
                }                                                                               \
                return 0;                                                                       \
        }

DEFINE_PUTC(cstdout)
DEFINE_PUTC(cstderr)

static int cstdin_getc(FILE *stream)
{
        uint8_t byte = 0u;
        enum uart_result result = 0;


        if (cstdin_dev != NULL) {
                result = uart_read_byte(cstdin_dev, &byte, UART_FLAG_TEXT_MODE);
                if (result != UART_RESULT_OK)
                        return _FDEV_EOF;

        } else
                return _FDEV_EOF;


        return (int) byte;
}

void uart_bind_to_cstdin(struct uart *dev)
{
        if (stdin != NULL)
                fclose(stdin);

        cstdin_dev = dev;
        stdin = fdevopen(NULL, cstdin_getc);
}

void uart_bind_to_cstdout(struct uart *dev)
{
        if (stdout != NULL)
                fclose(stdout);

        cstdout_dev = dev;
        stdout = fdevopen(cstdout_putc, NULL);
}

void uart_bind_to_cstderr(struct uart *dev)
{
        if (stderr != NULL)
                fclose(stderr);

        cstderr_dev = dev;
        stderr = fdevopen(cstderr_putc, NULL);
}

static inline __attribute__((always_inline)) void isr_udre_handler(struct uart_hw *hw)
{
        uint8_t byte = 0u;


        if (fifo_buffer_get_byte(&hw->tx_fifo, &byte))
                *( hw->reg.udrx_addr ) = byte;

        else
                uart_hw_intr_tx_disable(hw);
}

#define DEFINE_UDRE_ISR(dev_num)                                \
        ISR(USART##dev_num##_UDRE_vect)                         \
        {                                                       \
                isr_udre_handler(&hw_devices[UART##dev_num]);   \
        }

DEFINE_UDRE_ISR(0)
DEFINE_UDRE_ISR(1)
DEFINE_UDRE_ISR(2)
DEFINE_UDRE_ISR(3)

static inline __attribute__((always_inline)) void isr_rx_handler(struct uart_hw *hw)
{
        uint8_t byte = 0u;


        byte = *( hw->reg.udrx_addr );

        if (!fifo_buffer_put_byte(&hw->rx_fifo, byte))
                uart_hw_intr_rx_disable(hw);
}

#define DEFINE_RX_ISR(dev_num)                                  \
        ISR(USART##dev_num##_RX_vect)                           \
        {                                                       \
                isr_rx_handler(&hw_devices[UART##dev_num]);     \
        }

DEFINE_RX_ISR(0)
DEFINE_RX_ISR(1)
DEFINE_RX_ISR(2)
DEFINE_RX_ISR(3)

