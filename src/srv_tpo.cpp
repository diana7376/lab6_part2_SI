/* ==========================================================================
 *  srv_tpo.cpp
 *  Implementation of the Time-Proportional Output service. Tracks elapsed
 *  time inside a sliding window and decides whether the relay should be
 *  ON or OFF at the moment of evaluation.
 * ========================================================================== */
#include "srv_tpo.h"

extern "C" void srv_tpo_init(srv_tpo_t *tpo, uint32_t window_ms, uint32_t now_ms)
{
    if (tpo == (srv_tpo_t *)0) {
        return;
    }
    tpo->window_ms       = window_ms;
    tpo->window_start_ms = now_ms;
    tpo->duty_pct        = 0.0f;
}

extern "C" void srv_tpo_set_duty(srv_tpo_t *tpo, float duty_pct)
{
    if (tpo == (srv_tpo_t *)0) {
        return;
    }
    if (duty_pct < 0.0f)   { duty_pct = 0.0f;   }
    if (duty_pct > 100.0f) { duty_pct = 100.0f; }
    tpo->duty_pct = duty_pct;
}

extern "C" int srv_tpo_eval(srv_tpo_t *tpo, uint32_t now_ms)
{
    if (tpo == (srv_tpo_t *)0 || tpo->window_ms == 0u) {
        return 0;
    }

    /* Advance to the most recent window boundary. Works correctly even if
     * we missed one or more windows (e.g. task was preempted).             */
    uint32_t elapsed = now_ms - tpo->window_start_ms;
    while (elapsed >= tpo->window_ms) {
        tpo->window_start_ms += tpo->window_ms;
        elapsed             -= tpo->window_ms;
    }

    /* How many ms of ON-time correspond to the requested duty?             */
    uint32_t on_ms = (uint32_t)((tpo->duty_pct / 100.0f) * (float)tpo->window_ms);

    return (elapsed < on_ms) ? 1 : 0;
}
