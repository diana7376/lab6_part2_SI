/* ==========================================================================
 *  ed_relay.cpp
 *  Active-LOW relay driver. "LOW on the Arduino pin" = "coil energised" =
 *  "switch closed" = load (heater) receives power.
 * ========================================================================== */
#include "ed_relay.h"
#include "app_config.h"

#include <Arduino.h>

static ed_relay_state_t s_state = ED_RELAY_OFF;

extern "C" void ed_relay_setup(void)
{
    pinMode(APP_PIN_RELAY, OUTPUT);
    /* Boot into the safe state (heater OFF).
     * Active-LOW -> HIGH level means the relay coil is NOT energised.      */
    digitalWrite(APP_PIN_RELAY, HIGH);
    s_state = ED_RELAY_OFF;
}

extern "C" void ed_relay_set(ed_relay_state_t state)
{
    s_state = state;
    if (state == ED_RELAY_ON) {
        /* Active-LOW module: LOW energises the coil -> contact closed -> heater ON. */
        digitalWrite(APP_PIN_RELAY, LOW);
    } else {
        /* HIGH -> coil released -> contact open -> heater OFF.              */
        digitalWrite(APP_PIN_RELAY, HIGH);
    }
}

extern "C" ed_relay_state_t ed_relay_get_state(void)
{
    return s_state;
}
