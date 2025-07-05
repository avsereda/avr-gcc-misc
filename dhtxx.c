#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

#include "dhtxx.h"

void dhtxx_init(struct dhtxx *dht, struct gpio *dht_port)
{
        memset(dht, 0, sizeof(struct dhtxx));

        dht->port = dht_port;
}

static inline void usleep(unsigned long usecs)
{
        _delay_us((double) usecs);
}

static inline void sda_high(struct dhtxx *dht)
{
        /*
         * NOTE: We use external pullup resistor, so when we change
         *       port direction to input, bus automatically pulled up to VCC...
         */
        gpio_set_direction(dht->port, GPIO_DIRECTION_INPUT);
}

static inline void sda_low(struct dhtxx *dht)
{
        gpio_write(dht->port, GPIO_STATE_LOW);
}

static inline bool sda_expect(struct dhtxx *dht, enum gpio_state state,
                              unsigned long max_ticks)
{
        unsigned long tick_count = 0u;


        if (gpio_read(dht->port) != state)
                return false;

        while (gpio_read(dht->port) == state) {
                usleep(1u);
                if (tick_count++ >= max_ticks)
                        return false;
        }


        return true;
}

static inline bool sda_read(struct dhtxx *dht, int *result)
{
        *result = 0;

        if (sda_expect(dht, GPIO_STATE_LOW, 50u)) {
                usleep(26u);
                if (gpio_read(dht->port) == GPIO_STATE_LOW)
                        return true;

                *result = 1;
                return sda_expect(dht, GPIO_STATE_HIGH, 40u);
        }

        return false;
}

static inline bool data_checksum(uint8_t *data)
{
        uint8_t sum = 0u;


        sum = data[0] + data[1] + data[2] + data[3];
        if (sum == data[4])
                return true;


        return false;
}

static inline enum dhtxx_result sda_read_data(struct dhtxx *dht)
{
        enum dhtxx_result result = DHTXX_RESULT_SUCCESS;
        int i = 0;
        int byte = 0;
        uint8_t data[5] = {0, };
        int shift = 8;
        int bit_value = 0;


        for (; i < 40; ++i) {
                if (!sda_read(dht, &bit_value)) {
                        result = DHTXX_RESULT_NO_RESPONSE;
                        break;
                }

                if (shift == 0) {
                        shift = 8;
                        byte++;
                }

                data[byte] |= (bit_value << --shift);
        }

        if (data_checksum(data)) {
                dht->raw_humidity = (data[0] << 8) | data[1];
                dht->raw_temperature = (data[2] << 8) | data[3];
        } else
                result = DHTXX_RESULT_BAD_CHECKSUM;


        return result;
}

static inline bool sda_begin_poll(struct dhtxx *dht)
{
        sda_low(dht);
        usleep(800u);
        sda_high(dht);

        if (sda_expect(dht, GPIO_STATE_HIGH, 20u)) {
                if (sda_expect(dht, GPIO_STATE_LOW, 80u)
                                && sda_expect(dht, GPIO_STATE_HIGH, 80u))
                        return true;

        }

        return false;
}

enum dhtxx_result dhtxx_poll(struct dhtxx *dht)
{
        enum dhtxx_result result = DHTXX_RESULT_NO_RESPONSE;


#ifdef DHTXX_STRICT_TIMINGS
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
                if (sda_begin_poll(dht))
                        result = sda_read_data(dht);

#ifdef DHTXX_STRICT_TIMINGS
        }
#endif


        return result;
}

double dhtxx_get_temperature(struct dhtxx *dht)
{
        return ((double) dht->raw_temperature) / 10.0;
}

double dhtxx_get_humidity(struct dhtxx *dht)
{
        return ((double) dht->raw_humidity) / 10.0;
}

bool dhtxx_data_json_stringify(struct dhtxx *dht, char *buf, size_t buf_size)
{
        return ((size_t) snprintf_P(buf, buf_size,
                                    PSTR("{\"temperature\":%f,\"humidity\":%f}"),
                                    dhtxx_get_temperature(dht),
                                    dhtxx_get_humidity(dht))) <= buf_size;
}
