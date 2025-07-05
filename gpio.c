#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "gpio.h"

static struct gpio_addr_table addr_tables[] = {
        [0] = { &PINA, &PORTA, &DDRA },
        [1] = { &PINB, &PORTB, &DDRB },
        [2] = { &PINC, &PORTC, &DDRC },
        [3] = { &PIND, &PORTD, &DDRD },
        [4] = { &PINE, &PORTE, &DDRE },
        [5] = { &PINF, &PORTF, &DDRF },
        [6] = { &PING, &PORTG, &DDRG },
        [7] = { &PINH, &PORTH, &DDRH },
        [9] = { &PINJ, &PORTJ, &DDRJ },
        [10] = { &PINK, &PORTK, &DDRK },
        [11] = { &PINL, &PORTL, &DDRL }
};

static inline bool is_valid_port_letter(int port_letter)
{
        static char const port_letters[] PROGMEM = "ABCDEFGHJKL";
        size_t i = 0u;


        for (; i < sizeof(port_letters)/sizeof(char); ++i) {
                if (pgm_read_byte_near(&port_letters[i]) == port_letter)
                        return true;

        }


        return false;
}

bool gpio_init(struct gpio *port, const char *params)
{
        char port_letter = 0;
        unsigned port_bit = 0u;


        if (params == NULL)
                return false;

        if (sscanf_P(params, PSTR("PORT%c:%u"), &port_letter, &port_bit) != 2)
                return false;

        if (is_valid_port_letter(port_letter) && port_bit <= 7) {
                port->addr_table = &addr_tables[port_letter - 'A'];
                port->bit = port_bit;
                port->direction = GPIO_DIRECTION_INPUT;
                return true;
        }


        return false;
}

bool gpio_init_P(struct gpio *port, const char *params)
{
        char buf[10] = {0, };


        strncpy_P(buf, params, 9);
        return gpio_init(port, buf);
}

void gpio_set_direction(struct gpio *port, enum gpio_direction dir)
{
        volatile uint8_t *ddr_addr = NULL;


        ddr_addr = port->addr_table->ddr_addr;
        if (dir == GPIO_DIRECTION_INPUT)
                *ddr_addr &= (uint8_t) ~(_BV(port->bit));
        else
                *ddr_addr |= (uint8_t) _BV(port->bit);

        port->direction = dir;
}

enum gpio_state gpio_read(struct gpio *port)
{
        uint8_t value = 0u;
        const volatile uint8_t *pin_addr = NULL;


        if (gpio_get_direction(port) != GPIO_DIRECTION_INPUT)
                gpio_set_direction(port, GPIO_DIRECTION_INPUT);

        pin_addr = port->addr_table->pin_addr;
        value = *pin_addr;
        if (value & (uint8_t)(_BV(port->bit)))
                return GPIO_STATE_HIGH;


        return GPIO_STATE_LOW;
}

void gpio_write(struct gpio *port, enum gpio_state state)
{
        volatile uint8_t *port_addr = NULL;


        if (gpio_get_direction(port) != GPIO_DIRECTION_OUTPUT)
                gpio_set_direction(port, GPIO_DIRECTION_OUTPUT);

        port_addr = port->addr_table->port_addr;
        if (state == GPIO_STATE_HIGH)
                *port_addr |= (uint8_t) _BV(port->bit);
        else
                *port_addr &= (uint8_t) ~(_BV(port->bit));
}
