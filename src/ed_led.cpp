/* ==========================================================================
 *  ed_led.cpp
 *  Implementation of the LED driver. Uses a simple ID -> pin lookup table
 *  so adding another LED is just a matter of extending the enum and the
 *  pin map.
 * ========================================================================== */
#include "ed_led.h"
#include "app_config.h"

#include <Arduino.h>
#include <stdint.h>

/* ID -> Arduino pin map. Order MUST match the enum in ed_led.h.            */
static const uint8_t s_pin_map[ED_LED_ID_NR_OF] = {
    APP_PIN_LED_GREEN,  /* ED_LED_ID_GREEN */
    APP_PIN_LED_RED     /* ED_LED_ID_RED   */
};

extern "C" void ed_led_setup(void)
{
    for (int i = 0; i < ED_LED_ID_NR_OF; i++) {
        pinMode(s_pin_map[i], OUTPUT);
        digitalWrite(s_pin_map[i], LOW);
    }
}

extern "C" void ed_led_set(int led_id, ed_led_state_t state)
{
    if (led_id < 0 || led_id >= ED_LED_ID_NR_OF) {
        return;
    }
    digitalWrite(s_pin_map[led_id], (state == ED_LED_ON) ? HIGH : LOW);
}
