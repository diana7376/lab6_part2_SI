/* ==========================================================================
 *  main.cpp
 *  Lab 6 - Part 2 - Variant A : PID Temperature Control with hysteresis-free
 *  closed-loop regulation, DHT11 sensor, relay-driven resistor heater.
 *
 *  Architecture: cooperative time-triggered scheduler in loop().
 *  Each "task" runs at its own period using millis() timing. Equivalent to
 *  three FreeRTOS tasks but without the overhead of a preemptive kernel,
 *  the heap, or per-task stacks. Functionally identical for periodic
 *  control loops because all three "tasks" run to completion in well under
 *  their period.
 *
 *  Layered architecture (MCAL/ECAL/SRV/APP):
 *    APP  - this file (scheduler + state)
 *    SRV  - srv_pid (PID controller), srv_tpo (Time-Proportional Output)
 *    ECAL - ed_dht (sensor), ed_relay (actuator), ed_led (indicator)
 *
 *  Output is one human-readable status line per APP_T_DISP_MS, sent over
 *  the standard serial port at 9600 baud. STDIO printf is redirected to
 *  Serial via mcal_stdio in setup() so any module can call printf().
 * ========================================================================== */
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_config.h"
#include "ed_dht.h"
#include "ed_relay.h"
#include "ed_led.h"
#include "srv_pid.h"
#include "srv_tpo.h"

/* --- printf -> Serial redirect (so STDIO works as required) --- */
static int put_to_serial(char c, FILE *)
{
    if (c == '\n') Serial.write('\r');
    Serial.write(c);
    return 0;
}
static FILE s_stdout;

/* --- shared application state --- */
static srv_pid_t g_pid;
static srv_tpo_t g_tpo;

static float g_setpoint      = APP_PID_SETPOINT_C;
static float g_temperature_c = 0.0f;
static int   g_temp_valid    = 0;
static float g_pid_output    = 0.0f;
static int   g_relay_on      = 0;

/* --- "task" period trackers (ms) --- */
static unsigned long t_last_acq  = 0;
static unsigned long t_last_ctrl = 0;
static unsigned long t_last_disp = 0;

/* --- serial setpoint input buffer --- */
static char    s_rx_buf[8];
static uint8_t s_rx_len = 0;

/* ------------------------------------------------------------------------- */
/*  Print a float with 2 decimals using only integer math.
 *  Avoids dtostrf() and Serial.print(float, 2) which on AVR pull in heavy
 *  format machinery.
 * ------------------------------------------------------------------------- */
static void print_fixed2(float v)
{
    long centi = (long)(v * 100.0f + (v >= 0.0f ? 0.5f : -0.5f));
    if (centi < 0) { Serial.print('-'); centi = -centi; }
    Serial.print(centi / 100);
    Serial.print('.');
    long frac = centi % 100;
    if (frac < 10) Serial.print('0');
    Serial.print(frac);
}

/* ------------------------------------------------------------------------- */
/*  Equivalent of "Task_Acquisition" (FreeRTOS).
 *  Reads DHT11 every APP_T_ACQ_MS milliseconds.
 * ------------------------------------------------------------------------- */
static void run_acquisition(void)
{
    float t = 0.0f;
    if (ed_dht_read_temperature(&t)) {
        g_temperature_c = t;
        g_temp_valid    = 1;
    } else {
        g_temp_valid    = 0;
    }
}

/* ------------------------------------------------------------------------- */
/*  Equivalent of "Task_Control" (FreeRTOS).
 *  Runs PID -> TPO every APP_T_CTRL_MS milliseconds. Drives relay + LEDs.
 * ------------------------------------------------------------------------- */
static void run_control(void)
{
    const float dt_s = ((float)APP_T_CTRL_MS) / 1000.0f;

    if (g_temp_valid) {
        g_pid_output = srv_pid_compute(&g_pid, g_setpoint,
                                       g_temperature_c, dt_s);
    } else {
        /* Safety: if sensor unreadable, force OFF and reset the PID so we
         * start clean when sensor returns.                                 */
        g_pid_output = 0.0f;
        srv_pid_reset(&g_pid);
    }

    srv_tpo_set_duty(&g_tpo, g_pid_output);
    g_relay_on = srv_tpo_eval(&g_tpo, (uint32_t)millis());

    ed_relay_set(g_relay_on ? ED_RELAY_ON : ED_RELAY_OFF);
    ed_led_set(ED_LED_ID_GREEN, g_relay_on ? ED_LED_ON  : ED_LED_OFF);
    ed_led_set(ED_LED_ID_RED,   g_relay_on ? ED_LED_OFF : ED_LED_ON);
}

/* ------------------------------------------------------------------------- */
/*  Equivalent of "Task_Display" (FreeRTOS).
 *  Prints one status line every APP_T_DISP_MS milliseconds.
 * ------------------------------------------------------------------------- */
static void run_display(void)
{
    Serial.print(F("SP="));
    print_fixed2(g_setpoint);
    Serial.print(F("  T="));
    if (g_temp_valid) {
        print_fixed2(g_temperature_c);
    } else {
        Serial.print(F("?.??"));
    }
    Serial.print(F("  Out="));
    print_fixed2(g_pid_output);
    Serial.print(F("%  Relay="));
    Serial.println(g_relay_on ? F("ON ") : F("OFF"));
}

/* ------------------------------------------------------------------------- */
/*  Reads characters from Serial without blocking. On newline, parses the
 *  accumulated string as a float and updates g_setpoint if in range.
 *  Usage: type a number (e.g. "28.5") and press Enter.
 * ------------------------------------------------------------------------- */
static void run_serial_input(void)
{
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (s_rx_len > 0) {
                s_rx_buf[s_rx_len] = '\0';
                float val = (float)atof(s_rx_buf);
                if (val >= 10.0f && val <= 50.0f) {
                    g_setpoint = val;
                    srv_pid_reset(&g_pid);
                    Serial.print(F("SP -> "));
                    print_fixed2(g_setpoint);
                    Serial.println(F(" C"));
                } else {
                    Serial.println(F("ERR: setpoint must be 10-50 C"));
                }
                s_rx_len = 0;
            }
        } else if (s_rx_len < (uint8_t)(sizeof(s_rx_buf) - 1)) {
            s_rx_buf[s_rx_len++] = c;
        }
    }
}

/* ========================================================================= */
void setup(void)
{
    /* Serial up. Long delay so host USB enumerates before banner.          */
    Serial.begin(APP_SERIAL_BAUD);
    delay(2000);

    /* Bind printf to Serial so any module can use STDIO.                   */
    fdev_setup_stream(&s_stdout, put_to_serial, NULL, _FDEV_SETUP_WRITE);
    stdout = &s_stdout;

    /* Banner                                                               */
    Serial.println();
    Serial.println(F("======================================================="));
    Serial.println(F(" Lab 6 - Part 2 Var A : PID Temperature Control"));
    Serial.println(F(" Cooperative scheduler, DHT11 + Relay + LEDs"));
    Serial.println(F("======================================================="));
    Serial.print  (F("SetPoint  : "));
    print_fixed2(APP_PID_SETPOINT_C);
    Serial.println(F(" C"));
    Serial.print  (F("Pins      : DHT="));  Serial.print(APP_PIN_DHT);
    Serial.print  (F("  Relay="));          Serial.print(APP_PIN_RELAY);
    Serial.print  (F("  GREEN="));          Serial.print(APP_PIN_LED_GREEN);
    Serial.print  (F("  RED="));            Serial.println(APP_PIN_LED_RED);
    Serial.println(F("-------------------------------------------------------"));
    Serial.flush();

    /* Bring up drivers and services                                        */
    ed_dht_setup();
    ed_relay_setup();
    ed_led_setup();
    srv_pid_init(&g_pid,
                 APP_PID_KP, APP_PID_KI, APP_PID_KD,
                 APP_PID_OUT_MIN, APP_PID_OUT_MAX);
    srv_tpo_init(&g_tpo, APP_TPO_WINDOW_MS, (uint32_t)millis());

    /* Initial safe state: relay OFF, red LED on, green LED off             */
    ed_relay_set(ED_RELAY_OFF);
    ed_led_set(ED_LED_ID_GREEN, ED_LED_OFF);
    ed_led_set(ED_LED_ID_RED,   ED_LED_ON);

    Serial.println(F("[OK ] all drivers initialised, entering main loop"));
    Serial.flush();

    /* DHT11 needs ~1 s after power-up before its first valid reading.      */
    delay(1500);

    unsigned long now = millis();
    t_last_acq  = now;
    t_last_ctrl = now;
    t_last_disp = now;
}

/* ========================================================================= */
void loop(void)
{
    unsigned long now = millis();

    /* Serial setpoint input — runs every iteration, non-blocking */
    run_serial_input();

    /* "Task" Acquisition */
    if (now - t_last_acq >= APP_T_ACQ_MS) {
        t_last_acq += APP_T_ACQ_MS;
        run_acquisition();
    }

    /* "Task" Control */
    if (now - t_last_ctrl >= APP_T_CTRL_MS) {
        t_last_ctrl += APP_T_CTRL_MS;
        run_control();
    }

    /* "Task" Display */
    if (now - t_last_disp >= APP_T_DISP_MS) {
        t_last_disp += APP_T_DISP_MS;
        run_display();
    }
}
