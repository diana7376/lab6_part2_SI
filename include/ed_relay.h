/* ==========================================================================
 *  ed_relay.h
 *  ECAL-layer driver for a single-channel, active-LOW relay module used to
 *  switch the resistor heater on and off.
 *
 *  The driver exposes ON/OFF semantics; the active-LOW inversion is
 *  handled internally so the application never has to think about it.
 * ========================================================================== */
#ifndef ED_RELAY_H
#define ED_RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ED_RELAY_OFF = 0,
    ED_RELAY_ON  = 1
} ed_relay_state_t;

/*  Initialise the relay pin as output and force the safe state (OFF).      */
void             ed_relay_setup(void);

/*  Command the relay to the given state.                                   */
void             ed_relay_set(ed_relay_state_t state);

/*  Return the last state commanded via ed_relay_set().                     */
ed_relay_state_t ed_relay_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* ED_RELAY_H */
