/*
 * sc_sm.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_sm.md.
 */
#include "sc_sm.h"

sc_status_t sc_sm_config_valid(const sc_sm_config_t *cfg)
{
    sc_status_t status;

    if ((cfg == NULL) || (cfg->table == NULL))
    {
        status = SC_ERR_NULL;                        /* LLR-SM-7 */
    }
    else if (cfg->n == 0u)
    {
        status = SC_ERR_PARAM;                       /* LLR-SM-8 */
    }
    else
    {
        status = SC_OK;                              /* LLR-SM-9 */
    }

    return status;
}

sc_status_t sc_sm_init(sc_sm_state_t *state, const sc_sm_config_t *cfg)
{
    sc_status_t status;

    if ((state == NULL) || (cfg == NULL))
    {
        status = SC_ERR_NULL;                        /* LLR-SM-6 */
    }
    else
    {
        state->state = cfg->initial;                 /* LLR-SM-1 */
        status = SC_OK;
    }

    return status;
}

uint16_t sc_sm_state(const sc_sm_state_t *state)
{
    uint16_t result;

    if (state == NULL)
    {
        result = (uint16_t)0xFFFFu;                  /* LLR-SM-10 */
    }
    else
    {
        result = state->state;                      /* LLR-SM-11 */
    }

    return result;
}

bool sc_sm_dispatch(const sc_sm_config_t *cfg,
                    sc_sm_state_t *state,
                    uint16_t event,
                    void *ctx)
{
    bool fired = false;

    if ((cfg == NULL) || (state == NULL))
    {
        fired = false;                              /* LLR-SM-5 */
    }
    else
    {
        uint16_t i;
        for (i = 0u; i < cfg->n; i++)               /* LLR-SM-2: bounded scan */
        {
            const sc_sm_transition *t = &cfg->table[i];
            if ((t->from == state->state) && (t->event == event))
            {
                if (t->action != NULL)
                {
                    t->action(ctx);                 /* LLR-SM-3: run action */
                }
                state->state = t->to;               /* LLR-SM-4: move */
                fired = true;
                break;                              /* first match wins */
            }
        }
    }

    return fired;
}
