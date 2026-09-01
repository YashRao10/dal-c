/*
 * sc_debounce.h -- integrator debounce for a noisy digital input.
 *
 * The output only follows the raw input after the raw input has
 * *disagreed* with the current output for `threshold` consecutive
 * updates. A single spurious sample (or a burst shorter than the
 * threshold) is rejected. Distinct from sc_hysteresis, which works on an
 * analogue threshold rather than a sample count.
 */
#ifndef DAL_C_SC_DEBOUNCE_H
#define DAL_C_SC_DEBOUNCE_H

#include "sc_common.h"

typedef struct
{
    uint8_t threshold;   /* consecutive disagreeing samples to accept, >= 1 */
} sc_debounce_config_t;

typedef struct
{
    uint8_t output;      /* debounced value, 0 or 1 */
    uint8_t counter;     /* disagreement run length  */
} sc_debounce_state_t;

/*
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM threshold == 0
 */
SC_NODISCARD sc_status_t sc_debounce_config_valid(const sc_debounce_config_t *cfg);

/* Initialise to a known output. SC_OK / SC_ERR_NULL. */
SC_NODISCARD sc_status_t sc_debounce_init(sc_debounce_state_t *state, bool initial);

/*
 * Feed one raw sample and return the debounced output. A NULL cfg or
 * state returns false without modifying the state. Assumes cfg has passed
 * sc_debounce_config_valid().
 */
bool sc_debounce_update(const sc_debounce_config_t *cfg,
                        sc_debounce_state_t *state,
                        bool raw);

#endif /* DAL_C_SC_DEBOUNCE_H */
