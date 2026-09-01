/*
 * sc_hysteresis.c -- implementation.
 *
 * Each block is tagged with the low-level requirement it satisfies; see
 * requirements/sc_hysteresis.md for the full text and traceability.
 */
#include "sc_hysteresis.h"

sc_status_t sc_hysteresis_config_valid(const sc_hysteresis_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                 /* LLR-HYS-7 */
    }
    else if (cfg->high_threshold < cfg->low_threshold)
    {
        status = SC_ERR_PARAM;                /* LLR-HYS-8 */
    }
    else
    {
        status = SC_OK;                       /* LLR-HYS-9 */
    }

    return status;
}

sc_status_t sc_hysteresis_init(sc_hysteresis_state_t *state)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                 /* LLR-HYS-10 */
    }
    else
    {
        state->asserted = false;             /* LLR-HYS-1  */
        status = SC_OK;
    }

    return status;
}

bool sc_hysteresis_update(const sc_hysteresis_config_t *cfg,
                          sc_hysteresis_state_t *state,
                          int32_t input)
{
    bool result;

    if ((cfg == NULL) || (state == NULL))
    {
        result = false;                      /* LLR-HYS-5: null-safe        */
    }
    else
    {
        if ((state->asserted == false) && (input >= cfg->high_threshold))
        {
            state->asserted = true;          /* LLR-HYS-2: rising edge      */
        }
        else if ((state->asserted == true) && (input <= cfg->low_threshold))
        {
            state->asserted = false;         /* LLR-HYS-3: falling edge     */
        }
        else
        {
            /* LLR-HYS-4: input is between the thresholds -- hold */
        }

        result = state->asserted;            /* LLR-HYS-6: output = state   */
    }

    return result;
}

bool sc_hysteresis_output(const sc_hysteresis_state_t *state)
{
    bool result;

    if (state == NULL)
    {
        result = false;                      /* LLR-HYS-11 */
    }
    else
    {
        result = state->asserted;            /* LLR-HYS-12 */
    }

    return result;
}
