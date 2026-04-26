/* ==========================================================================
 *  srv_pid.h
 *  SRV-layer Proportional-Integral-Derivative (PID) controller.
 *
 *  Direct-acting: error = SetPoint - Measured, so a positive error asks
 *  for MORE heat (the heater is a direct-acting actuator).
 *
 *  The context struct is opaque from the caller's point of view but kept
 *  in the header so the user can allocate it on the stack / statically.
 * ========================================================================== */
#ifndef SRV_PID_H
#define SRV_PID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Tuning */
    float kp;
    float ki;
    float kd;

    /* Output clamp */
    float out_min;
    float out_max;

    /* Internal state */
    float integral;
    float prev_error;
    int   first_run;
} srv_pid_t;

/*  Initialise a PID instance with gains and output saturation limits.
 *  Clears integral and previous-error state.                               */
void  srv_pid_init(srv_pid_t *pid,
                   float kp, float ki, float kd,
                   float out_min, float out_max);

/*  Clear integral accumulator and previous-error memory. Call whenever
 *  the loop is disabled or the setpoint changes abruptly.                  */
void  srv_pid_reset(srv_pid_t *pid);

/*  Run one PID iteration.
 *  @param setpoint  Desired value.
 *  @param measured  Current measured value.
 *  @param dt_s      Seconds elapsed since the previous call (must be > 0).
 *  @return Output clamped to [out_min, out_max] with conditional-integration
 *          anti-windup.                                                    */
float srv_pid_compute(srv_pid_t *pid,
                      float setpoint, float measured, float dt_s);

#ifdef __cplusplus
}
#endif

#endif /* SRV_PID_H */
