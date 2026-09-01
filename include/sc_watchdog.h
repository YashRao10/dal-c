/*
 * sc_watchdog.h -- deadline supervisor for a periodic activity.
 *
 * A supervised activity must check in ("kick") at least once every
 * `timeout` ticks. The application calls sc_watchdog_tick() once per system
 * tick; the supervised code calls sc_watchdog_kick() when it completes a
 * cycle. If `timeout` ticks elapse with no intervening kick, the watchdog
 * trips -- and stays tripped, a latched fault, until sc_watchdog_init()
 * re-arms it.
 *
 * Latching is deliberate. A missed deadline is a fault the system should
 * annunciate and hold, not one that clears itself the instant the late task
 * happens to run again.
 *
 * One small state struct per supervised activity. No dynamic memory, no
 * unbounded loop, no shared state between watchdogs.
 */
#ifndef DAL_C_SC_WATCHDOG_H
#define DAL_C_SC_WATCHDOG_H

#include "sc_common.h"

typedef enum
{
    SC_WATCHDOG_OK      = 0,  /* the deadline has not been missed            */
    SC_WATCHDOG_TRIPPED = 1   /* a deadline was missed; latched until re-arm */
} sc_watchdog_verdict_t;

typedef struct
{
    uint16_t timeout;   /* ticks without a kick that trip the watchdog, >= 1 */
} sc_watchdog_config_t;

typedef struct
{
    uint16_t elapsed;   /* ticks since the last kick (or since init)          */
    bool     tripped;   /* latched: set on a missed deadline, cleared by init */
} sc_watchdog_state_t;

/*
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM timeout == 0
 */
SC_NODISCARD sc_status_t sc_watchdog_config_valid(const sc_watchdog_config_t *cfg);

/* Re-arm: clear the elapsed count and the tripped flag. SC_OK / SC_ERR_NULL. */
SC_NODISCARD sc_status_t sc_watchdog_init(sc_watchdog_state_t *state);

/*
 * Check in. Resets the elapsed count so the deadline is measured afresh.
 * A NULL state is ignored. Does not clear a tripped flag -- only
 * sc_watchdog_init() does that.
 */
void sc_watchdog_kick(sc_watchdog_state_t *state);

/*
 * Advance one tick and report the verdict. A NULL cfg or state returns
 * SC_WATCHDOG_TRIPPED (fail-safe) without touching the state. Assumes cfg
 * has passed sc_watchdog_config_valid().
 *
 *   SC_WATCHDOG_OK       within the deadline
 *   SC_WATCHDOG_TRIPPED  the deadline has been missed (latched)
 */
sc_watchdog_verdict_t sc_watchdog_tick(const sc_watchdog_config_t *cfg,
                                       sc_watchdog_state_t *state);

/*
 * Poll the latched fault state without advancing a tick. Returns true iff
 * `state` is non-NULL and its watchdog has tripped.
 */
bool sc_watchdog_expired(const sc_watchdog_state_t *state);

#endif /* DAL_C_SC_WATCHDOG_H */
