/* ==========================================================================
 *  ed_led.h
 *  ECAL-layer driver for the indicator LEDs (green = heater ON,
 *  red = heater OFF). Multi-instance design based on LED IDs, following
 *  the pattern shown in the lab reference material (A.2 ed_button.h).
 * ========================================================================== */
#ifndef ED_LED_H
#define ED_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/*  LED identifiers. Keep ED_LED_ID_NR_OF as the last item.                 */
enum {
    ED_LED_ID_GREEN = 0,
    ED_LED_ID_RED,
    ED_LED_ID_NR_OF
};

typedef enum {
    ED_LED_OFF = 0,
    ED_LED_ON  = 1
} ed_led_state_t;

/*  Initialise all LED pins as outputs and force them OFF.                  */
void ed_led_setup(void);

/*  Command one LED ON or OFF. Unknown IDs are silently ignored.            */
void ed_led_set(int led_id, ed_led_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* ED_LED_H */
