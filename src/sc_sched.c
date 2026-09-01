/*
 * sc_sched.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_sched.md.
 */
#include "sc_sched.h"

sc_status_t sc_sched_config_valid(const sc_sched_config_t *cfg)
{
    sc_status_t status = SC_OK;

    if ((cfg == NULL) || (cfg->slots == NULL))
    {
        status = SC_ERR_NULL;                        /* LLR-SCH-6 */
    }
    else if (cfg->n == 0u)
    {
        status = SC_ERR_PARAM;                       /* LLR-SCH-7 */
    }
    else
    {
        uint16_t i;
        for (i = 0u; i < cfg->n; i++)                /* LLR-SCH-8 */
        {
            const sc_sched_slot *s = &cfg->slots[i];
            if (s->run == NULL)
            {
                status = SC_ERR_NULL;
            }
            else if ((s->period == 0u) || (s->phase >= s->period))
            {
                status = SC_ERR_PARAM;
            }
            else
            {
                /* slot ok */
            }
        }
    }

    return status;
}

sc_status_t sc_sched_init(sc_sched_state_t *state)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                        /* LLR-SCH-5 */
    }
    else
    {
        state->tick = 0u;                            /* LLR-SCH-1 */
        status = SC_OK;
    }

    return status;
}

uint32_t sc_sched_tick(const sc_sched_config_t *cfg,
                       sc_sched_state_t *state,
                       void *ctx)
{
    uint32_t ran = 0u;

    if ((cfg == NULL) || (state == NULL))
    {
        ran = 0u;                                    /* LLR-SCH-4 */
    }
    else
    {
        uint16_t i;
        for (i = 0u; i < cfg->n; i++)                /* LLR-SCH-2: table order */
        {
            const sc_sched_slot *s = &cfg->slots[i];
            if ((state->tick % s->period) == s->phase) /* LLR-SCH-3: due? */
            {
                s->run(ctx);
                ran++;
            }
        }
        state->tick++;                               /* LLR-SCH-9: advance */
    }

    return ran;
}
