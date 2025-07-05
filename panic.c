#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "gpio.h"
#include "panic.h"

static inline void disable_all_gpio_ports(void)
{
        DDRA = (uint8_t) 0u;
        DDRB = (uint8_t) 0u;
        DDRC = (uint8_t) 0u;
        DDRD = (uint8_t) 0u;
        DDRE = (uint8_t) 0u;
        DDRF = (uint8_t) 0u;
        DDRG = (uint8_t) 0u;
        DDRH = (uint8_t) 0u;
        DDRJ = (uint8_t) 0u;
        DDRK = (uint8_t) 0u;
        DDRL = (uint8_t) 0u;
}

void panic(void)
{
        struct gpio led_port;
        enum gpio_state led_state = GPIO_STATE_HIGH;


        wdt_disable();
        cli();

        disable_all_gpio_ports();

        gpio_init_P(&led_port, PSTR("PORTB:7"));

        for (;;) {
                gpio_write(&led_port, led_state);

                if (led_state == GPIO_STATE_HIGH)
                        led_state = GPIO_STATE_LOW;
                else
                        led_state = GPIO_STATE_HIGH;

                _delay_ms(70.0);
        }
}
