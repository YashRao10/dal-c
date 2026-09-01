/*
 * sc_debounce.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_debounce.md.
 */
#include "sc_debounce.h"

sc_status_t sc_debounce_config_valid(const sc_debounce_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                        /* LLR-DB-6 */
    }
    else if (cfg->threshold == 0u)
    {
        status = SC_ERR_PARAM;                       /* LLR-DB-7 */
    }
    else
    {
        status = SC_OK;                              /* LLR-DB-8 */
    }

    return status;
}

sc_status_t sc_debounce_init(sc_debounce_state_t *state, bool initial)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                        /* LLR-DB-9 */
    }
    else
    {
        state->output  = (uint8_t)(initial ? 1 : 0);   /* LLR-DB-1 */
        state->counter = 0u;
        status = SC_OK;
    }

    return status;
}

bool sc_debounce_update(const sc_debounce_config_t *cfg,
                        sc_debounce_state_t *state,
                        bool raw)
{
    bool result;

    if ((cfg == NULL) || (state == NULL))
    {
        result = false;                             /* LLR-DB-5 */
    }
    else
    {
        uint8_t raw_bit = (uint8_t)(raw ? 1 : 0);

        if (raw_bit == state->output)
        {
            state->counter = 0u;                    /* LLR-DB-2: agrees, reset run */
        }
        else
        {
            state->counter = (uint8_t)(state->counter + 1u);                       /* LLR-DB-3: disagrees */
            if (state->counter >= cfg->threshold)
            {
                state->output  = raw_bit;           /* LLR-DB-4: accept the change */
                state->counter = 0u;
            }
        }

        result = (state->output != 0u);
    }

    return result;
}
