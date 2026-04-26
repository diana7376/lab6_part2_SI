/* ==========================================================================
 *  app_config.h
 *  Centralised configuration for Lab 6 / Part 2 / Variant A.
 * ========================================================================== */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Pin map */
#define APP_PIN_DHT             7
#define APP_PIN_RELAY           8
#define APP_PIN_LED_GREEN       5
#define APP_PIN_LED_RED         6

/* PID configuration */
#define APP_PID_SETPOINT_C      22.5f
#define APP_PID_KP              8.0f
#define APP_PID_KI              0.15f
#define APP_PID_KD              2.0f
#define APP_PID_OUT_MIN         0.0f
#define APP_PID_OUT_MAX         100.0f

/* Time-Proportional Output window */
#define APP_TPO_WINDOW_MS       2000ul

/* Periods (ms) for each cooperative "task". Each runs at its own cadence
 * inside loop() using millis() timing - no FreeRTOS, no stacks, no heap. */
#define APP_T_ACQ_MS            2000ul   /* DHT11 cannot be polled faster than 1 Hz; 2 s is safe */
#define APP_T_CTRL_MS           100ul    /* PID + TPO tick                                     */
#define APP_T_DISP_MS           500ul    /* Serial status                                      */

#define APP_SERIAL_BAUD         9600ul

#endif
