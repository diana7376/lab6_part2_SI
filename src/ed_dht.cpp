/* ==========================================================================
 *  ed_dht.cpp
 *  Minimal hand-written DHT11 driver. Bit-bangs the 1-wire-like protocol
 *  directly on the data pin. Zero dependencies on Adafruit DHT.
 *
 *  Uses micros() for pulse measurement — immune to digitalRead() overhead
 *  variations across AVR pin/timer assignments.
 *
 *  No ATOMIC_BLOCK needed: the Timer0 ISR adds at most ~1 µs jitter per
 *  pulse measurement, negligible against the 44 µs gap between a 0-bit
 *  (~26 µs HIGH) and a 1-bit (~70 µs HIGH).
 * ========================================================================== */
#include "ed_dht.h"
#include "app_config.h"
#include <Arduino.h>

extern "C" void ed_dht_setup(void)
{
    pinMode(APP_PIN_DHT, INPUT_PULLUP);
}

static int dht11_read_raw(uint8_t out[5])
{
    /* 1. Start signal: drive LOW >= 18 ms, then release to pull-up. */
    pinMode(APP_PIN_DHT, OUTPUT);
    digitalWrite(APP_PIN_DHT, LOW);
    delay(20);
    digitalWrite(APP_PIN_DHT, HIGH);
    delayMicroseconds(30);
    pinMode(APP_PIN_DHT, INPUT_PULLUP);

    uint8_t bytes[5] = {0, 0, 0, 0, 0};
    unsigned long t;

    /* 2. Response: sensor pulls LOW ~80 µs then HIGH ~80 µs. */
    t = micros();
    while (digitalRead(APP_PIN_DHT) == HIGH)
        if (micros() - t > 500UL) return 0;

    t = micros();
    while (digitalRead(APP_PIN_DHT) == LOW)
        if (micros() - t > 500UL) return 0;

    t = micros();
    while (digitalRead(APP_PIN_DHT) == HIGH)
        if (micros() - t > 500UL) return 0;

    /* 3. Read 40 bits.
     *    Each bit: ~50 µs LOW gap, then HIGH ~26 µs ('0') or ~70 µs ('1'). */
    for (uint8_t i = 0; i < 40; i++) {
        t = micros();
        while (digitalRead(APP_PIN_DHT) == LOW)
            if (micros() - t > 200UL) return 0;

        t = micros();
        while (digitalRead(APP_PIN_DHT) == HIGH)
            if (micros() - t > 200UL) return 0;
        unsigned long pulse = micros() - t;

        bytes[i / 8] <<= 1;
        if (pulse > 40UL) bytes[i / 8] |= 1;   /* 40 µs midpoint: 0-bit ~26, 1-bit ~70 */
    }

    /* 4. Checksum (low 8 bits of sum of first 4 bytes). */
    if (((bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xFF) != bytes[4])
        return 0;

    out[0] = bytes[0]; out[1] = bytes[1]; out[2] = bytes[2];
    out[3] = bytes[3]; out[4] = bytes[4];
    return 1;
}

extern "C" int ed_dht_read_temperature(float *out_temp_c)
{
    if (!out_temp_c) return 0;
    uint8_t b[5];
    if (!dht11_read_raw(b)) return 0;
    *out_temp_c = (float)b[2] + (float)b[3] / 10.0f;
    return 1;
}

extern "C" int ed_dht_read_humidity(float *out_humidity)
{
    if (!out_humidity) return 0;
    uint8_t b[5];
    if (!dht11_read_raw(b)) return 0;
    *out_humidity = (float)b[0] + (float)b[1] / 10.0f;
    return 1;
}
