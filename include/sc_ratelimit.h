/*
 * sc_ratelimit.h -- slew-rate limiter with output clamp.
 *
 * Moves an internal output toward a commanded target by at most a fixed
 * step per update, then clamps it to a configured range. Used to keep a
 * command signal (actuator position, setpoint, brightness, ...) from
 * changing faster than downstream hardware can safely follow.
 *
 * All arithmetic is saturating 32-bit: extreme targets cannot wrap the
 * output. No 64-bit maths, no dynamic memory, no unbounded loops.
 */
#ifndef DAL_C_SC_RATELIMIT_H
#define DAL_C_SC_RATELIMIT_H

#include "sc_common.h"

/*
 * Immutable configuration. `max_step_up` and `max_step_down` are the
 * largest permitted increase / decrease per update and shall be >= 0.
 * `out_min` shall be <= `out_max`. Validate once with
 * sc_ratelimit_config_valid().
 */
typedef struct
{
    int32_t max_step_up;
    int32_t max_step_down;
    int32_t out_min;
    int32_t out_max;
} sc_ratelimit_config_t;

/* Mutable per-instance state. Initialise with sc_ratelimit_init(). */
typedef struct
{
    int32_t output;
} sc_ratelimit_state_t;

/*
 * Validate a configuration.
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM a step is negative, or out_min > out_max
 */
SC_NODISCARD sc_status_t sc_ratelimit_config_valid(const sc_ratelimit_config_t *cfg);

/*
 * Initialise state with a starting output value. The value is not clamped
 * here; the first sc_ratelimit_update() call pulls it into [out_min,out_max].
 *   SC_OK / SC_ERR_NULL
 */
SC_NODISCARD sc_status_t sc_ratelimit_init(sc_ratelimit_state_t *state, int32_t initial);

/*
 * Advance one update cycle toward `target` and return the new output.
 *
 * Preconditions: `cfg` and `state` are non-NULL and `cfg` has passed
 * sc_ratelimit_config_valid(). If `state` is non-NULL but `cfg` is NULL the
 * current output is returned unchanged; if `state` is NULL, 0 is returned.
 */
int32_t sc_ratelimit_update(const sc_ratelimit_config_t *cfg,
                            sc_ratelimit_state_t *state,
                            int32_t target);

/* Read the current output without advancing. Returns 0 if state is NULL. */
int32_t sc_ratelimit_output(const sc_ratelimit_state_t *state);

#endif /* DAL_C_SC_RATELIMIT_H */
