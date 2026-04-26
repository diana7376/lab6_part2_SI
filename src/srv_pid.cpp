/* ==========================================================================
 *  srv_pid.cpp
 *  Textbook parallel-form PID with conditional-integration anti-windup.
 *
 *      u(t) = Kp * e(t) + Ki * integral(e) + Kd * d/dt(e)
 *
 *  When the output saturates we roll back the latest contribution to the
 *  integral, which prevents integral windup during long periods of
 *  saturation (very common with a heater that is just always ON).
 * ========================================================================== */
#include "srv_pid.h"

extern "C" void srv_pid_init(srv_pid_t *pid,
                             float kp, float ki, float kd,
                             float out_min, float out_max)
{
    if (pid == (srv_pid_t *)0) {
        return;
    }
    pid->kp         = kp;
    pid->ki         = ki;
    pid->kd         = kd;
    pid->out_min    = out_min;
    pid->out_max    = out_max;
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
    pid->first_run  = 1;
}

extern "C" void srv_pid_reset(srv_pid_t *pid)
{
    if (pid == (srv_pid_t *)0) {
        return;
    }
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
    pid->first_run  = 1;
}

extern "C" float srv_pid_compute(srv_pid_t *pid,
                                 float setpoint, float measured, float dt_s)
{
    if (pid == (srv_pid_t *)0) {
        return 0.0f;
    }
    if (dt_s <= 0.0f) {
        dt_s = 0.001f;  /* defensive: avoid division by zero */
    }

    /* Error and I-term */
    float error = setpoint - measured;
    pid->integral += error * dt_s;

    /* D-term with a "bumpless" first-run behaviour: on the very first
     * iteration we do not have a previous error, so force derivative to 0
     * to avoid a large "kick" at startup.                                  */
    float derivative;
    if (pid->first_run) {
        derivative = 0.0f;
        pid->first_run = 0;
    } else {
        derivative = (error - pid->prev_error) / dt_s;
    }
    pid->prev_error = error;

    /* Raw output */
    float output = (pid->kp * error)
                 + (pid->ki * pid->integral)
                 + (pid->kd * derivative);

    /* Clamp + conditional-integration anti-windup */
    if (output > pid->out_max) {
        output = pid->out_max;
        pid->integral -= error * dt_s;   /* undo this integration step */
    } else if (output < pid->out_min) {
        output = pid->out_min;
        pid->integral -= error * dt_s;
    }

    return output;
}
