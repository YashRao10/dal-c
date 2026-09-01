/*
 * sc_ratelimit.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_ratelimit.md.
 */
#include "sc_ratelimit.h"
#include "sc_sat.h"

sc_status_t sc_ratelimit_config_valid(const sc_ratelimit_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                            /* LLR-RL-11 */
    }
    else if ((cfg->max_step_up < 0) || (cfg->max_step_down < 0))
    {
        status = SC_ERR_PARAM;                           /* LLR-RL-12 */
    }
    else if (cfg->out_min > cfg->out_max)
    {
        status = SC_ERR_PARAM;                           /* LLR-RL-13 */
    }
    else
    {
        status = SC_OK;                                  /* LLR-RL-14 */
    }

    return status;
}

sc_status_t sc_ratelimit_init(sc_ratelimit_state_t *state, int32_t initial)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                            /* LLR-RL-15 */
    }
    else
    {
        state->output = initial;                        /* LLR-RL-16 */
        status = SC_OK;
    }

    return status;
}

int32_t sc_ratelimit_update(const sc_ratelimit_config_t *cfg,
                            sc_ratelimit_state_t *state,
                            int32_t target)
{
    int32_t result;

    if (state == NULL)
    {
        result = 0;                                     /* LLR-RL-6 */
    }
    else if (cfg == NULL)
    {
        result = state->output;                         /* LLR-RL-6 */
    }
    else
    {
        int32_t delta = sc_sat_sub_i32(target, state->output);   /* LLR-RL-1 */
        int32_t next;

        if (delta > cfg->max_step_up)
        {
            delta = cfg->max_step_up;                    /* LLR-RL-2 */
        }
        else if (delta < -cfg->max_step_down)
        {
            delta = -cfg->max_step_down;                 /* LLR-RL-3 */
        }
        else
        {
            /* LLR-RL-4: change is already within the slew limits */
        }

        next = sc_sat_add_i32(state->output, delta);     /* LLR-RL-5 */

        if (next < cfg->out_min)
        {
            next = cfg->out_min;                         /* LLR-RL-7 */
        }
        else if (next > cfg->out_max)
        {
            next = cfg->out_max;                         /* LLR-RL-8 */
        }
        else
        {
            /* LLR-RL-17: already within the clamp range */
        }

        state->output = next;
        result = state->output;                          /* LLR-RL-9 */
    }

    return result;
}

int32_t sc_ratelimit_output(const sc_ratelimit_state_t *state)
{
    int32_t result;

    if (state == NULL)
    {
        result = 0;                                     /* LLR-RL-18 */
    }
    else
    {
        result = state->output;                         /* LLR-RL-19 */
    }

    return result;
}
