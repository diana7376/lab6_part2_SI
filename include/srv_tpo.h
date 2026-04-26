/* ==========================================================================
 *  srv_tpo.h
 *  SRV-layer Time-Proportional Output (TPO).
 *
 *  Converts a 0..100 % continuous control signal (from the PID) into a
 *  relay ON/OFF waveform inside a fixed time window. Example with a 2000 ms
 *  window and a 60 % duty:
 *
 *      | ON for 1200 ms | OFF for 800 ms | ON for 1200 ms | OFF for 800 ms | ...
 *
 *  This is the standard way to use a relay as a "pseudo-PWM" actuator
 *  without destroying its contacts with fast switching.
 * ========================================================================== */
#ifndef SRV_TPO_H
#define SRV_TPO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t window_ms;        /* Total window length in ms               */
    uint32_t window_start_ms;  /* millis() value at which current window began */
    float    duty_pct;         /* Currently requested duty 0..100         */
} srv_tpo_t;

/*  Initialise a TPO instance with a window length.
 *  @param now_ms  Current millis() reading; used to anchor the window.   */
void srv_tpo_init(srv_tpo_t *tpo, uint32_t window_ms, uint32_t now_ms);

/*  Change the requested duty cycle. Takes effect in the current window
 *  without waiting for the next one.                                     */
void srv_tpo_set_duty(srv_tpo_t *tpo, float duty_pct);

/*  Evaluate the TPO for the current moment and return the desired relay
 *  state: 1 = ON, 0 = OFF.                                               */
int  srv_tpo_eval(srv_tpo_t *tpo, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* SRV_TPO_H */
