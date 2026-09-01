/*
 * sc_hysteresis.h -- bounded hysteresis comparator (Schmitt trigger)
 *
 * A single-input Boolean comparator with independent assert and clear
 * thresholds, so a noisy or slowly-varying signal near the trip point
 * cannot make the output chatter.
 *
 * Output convention: the output ASSERTS (true) once the input rises to or
 * above `high_threshold`, and CLEARS (false) once the input falls to or
 * below `low_threshold`. Between the two thresholds the output holds its
 * previous value. This is the classic rising-edge Schmitt trigger; an
 * "assert while the signal is LOW" response (e.g. a low-fuel warning) is
 * obtained by negating the input and thresholds at the call site.
 */
#ifndef DAL_C_SC_HYSTERESIS_H
#define DAL_C_SC_HYSTERESIS_H

#include "sc_common.h"

/*
 * Immutable configuration. `high_threshold` shall be >= `low_threshold`;
 * equal thresholds are permitted and give a zero-width (plain comparator)
 * response. Validate once with sc_hysteresis_config_valid() before use.
 */
typedef struct
{
    int32_t low_threshold;
    int32_t high_threshold;
} sc_hysteresis_config_t;

/* Mutable per-instance state. Initialise with sc_hysteresis_init(). */
typedef struct
{
    bool asserted;
} sc_hysteresis_state_t;

/*
 * Validate a configuration.
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM high_threshold < low_threshold
 */
SC_NODISCARD sc_status_t sc_hysteresis_config_valid(const sc_hysteresis_config_t *cfg);

/*
 * Initialise state to the CLEARED output.
 *   SC_OK        done
 *   SC_ERR_NULL  state is NULL
 */
SC_NODISCARD sc_status_t sc_hysteresis_init(sc_hysteresis_state_t *state);

/*
 * Advance one update cycle and return the new output value.
 *
 * Preconditions: `cfg` and `state` are non-NULL and `cfg` has passed
 * sc_hysteresis_config_valid(). The caller owns that check; this function
 * does not repeat it on the hot path. If either pointer is NULL the
 * function returns false without dereferencing it.
 */
bool sc_hysteresis_update(const sc_hysteresis_config_t *cfg,
                          sc_hysteresis_state_t *state,
                          int32_t input);

/* Read the current output without advancing. Returns false if state is NULL. */
bool sc_hysteresis_output(const sc_hysteresis_state_t *state);

#endif /* DAL_C_SC_HYSTERESIS_H */
