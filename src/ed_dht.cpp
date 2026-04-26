/* ==========================================================================
 *  ed_dht.cpp
 *  Minimal hand-written DHT11 driver. Bit-bangs the 1-wire-like protocol
 *  directly on the data pin. Zero dependencies on Adafruit DHT or
 *  Adafruit_Sensor (which pull in Serial.print(float) paths and
 *  significantly increase stack pressure under FreeRTOS on AVR).
 *
 *  Protocol overview:
 *    1. Master pulls line LOW for >= 18 ms (start signal).
 *    2. Master releases line; pull-up brings it HIGH.
 *    3. Sensor pulls LOW ~80 us, then HIGH ~80 us (response).
 *    4. Sensor sends 40 bits: each bit is a ~50 us LOW followed by HIGH;
 *       a HIGH lasting ~26 us = bit 0, ~70 us = bit 1.
 *    5. The 40 bits = humidity_int, humidity_dec, temp_int, temp_dec,
 *       checksum (sum of the first 4 bytes, low 8 bits).
 *
 *  All timing is done with cli()/sei() to avoid FreeRTOS tick interrupt
 *  jitter corrupting the sub-microsecond bit measurements.
 * ========================================================================== */
#include "ed_dht.h"
#include "app_config.h"

#include <Arduino.h>
#include <util/atomic.h>

extern "C" void ed_dht_setup(void)
{
    /* Configure pin as input with the AVR internal pull-up.
     * The DHT11 module on a 3-pin breakout already has its own pull-up;
     * adding ours in parallel does no harm.                              */
    pinMode(APP_PIN_DHT, INPUT_PULLUP);
}

/*  Read all 5 raw bytes from the sensor into out[5].
 *  Returns 1 on success, 0 on any timeout / checksum failure.            */
static int dht11_read_raw(uint8_t out[5])
{
    /* 1. Send start signal: pull LOW for >= 20 ms, then release. */
    pinMode(APP_PIN_DHT, OUTPUT);
    digitalWrite(APP_PIN_DHT, LOW);
    delay(20);
    digitalWrite(APP_PIN_DHT, HIGH);
    delayMicroseconds(40);
    pinMode(APP_PIN_DHT, INPUT_PULLUP);

    /* From here on, timing is critical. Disable interrupts so neither the
     * Arduino timer nor the FreeRTOS tick can preempt the bit measurement. */
    uint8_t bytes[5] = {0, 0, 0, 0, 0};
    int ok = 1;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

        /* 2. Wait for sensor response: LOW ~80 us, then HIGH ~80 us. */
        uint16_t t = 0;
        while (digitalRead(APP_PIN_DHT) == HIGH) {
            if (++t > 200) { ok = 0; break; }
            delayMicroseconds(1);
        }
        if (ok) {
            t = 0;
            while (digitalRead(APP_PIN_DHT) == LOW) {
                if (++t > 200) { ok = 0; break; }
                delayMicroseconds(1);
            }
        }
        if (ok) {
            t = 0;
            while (digitalRead(APP_PIN_DHT) == HIGH) {
                if (++t > 200) { ok = 0; break; }
                delayMicroseconds(1);
            }
        }

        /* 3. Read 40 bits. Each bit starts with ~50 us LOW; the HIGH that
         *    follows is short for "0" (~26 us) or long for "1" (~70 us). */
        if (ok) {
            for (uint8_t i = 0; i < 40 && ok; i++) {

                /* wait through LOW phase */
                t = 0;
                while (digitalRead(APP_PIN_DHT) == LOW) {
                    if (++t > 100) { ok = 0; break; }
                    delayMicroseconds(1);
                }
                if (!ok) break;

                /* measure HIGH phase length */
                uint16_t h = 0;
                while (digitalRead(APP_PIN_DHT) == HIGH) {
                    if (++h > 100) { ok = 0; break; }
                    delayMicroseconds(1);
                }
                if (!ok) break;

                /* HIGH > ~40 us means "1", else "0" */
                bytes[i / 8] <<= 1;
                if (h > 40) {
                    bytes[i / 8] |= 1;
                }
            }
        }
    } /* interrupts re-enabled */

    if (!ok) return 0;

    /* 4. Verify checksum. */
    uint8_t sum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
    if (sum != bytes[4]) return 0;

    out[0] = bytes[0];
    out[1] = bytes[1];
    out[2] = bytes[2];
    out[3] = bytes[3];
    out[4] = bytes[4];
    return 1;
}

extern "C" int ed_dht_read_temperature(float *out_temp_c)
{
    if (out_temp_c == (float *)0) return 0;
    uint8_t b[5];
    if (!dht11_read_raw(b)) return 0;
    /* DHT11: byte 2 = temp integer, byte 3 = temp tenths. */
    *out_temp_c = (float)b[2] + (float)b[3] / 10.0f;
    return 1;
}

extern "C" int ed_dht_read_humidity(float *out_humidity)
{
    if (out_humidity == (float *)0) return 0;
    uint8_t b[5];
    if (!dht11_read_raw(b)) return 0;
    *out_humidity = (float)b[0] + (float)b[1] / 10.0f;
    return 1;
}
