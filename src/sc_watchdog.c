/*
 * sc_watchdog.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_watchdog.md.
 *
 * The elapsed count only ever advances while it is below `timeout`, so it
 * settles at `timeout` and the uint16_t increment can never wrap. The
 * tripped flag is written in exactly one place and cleared in exactly one
 * other (sc_watchdog_init); that is the whole latch.
 */
#include "sc_watchdog.h"

sc_status_t sc_watchdog_config_valid(const sc_watchdog_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                            /* LLR-WDG-1 */
    }
    else if (cfg->timeout == 0u)
    {
        status = SC_ERR_PARAM;                           /* LLR-WDG-2 */
    }
    else
    {
        status = SC_OK;                                  /* LLR-WDG-3 */
    }

    return status;
}

sc_status_t sc_watchdog_init(sc_watchdog_state_t *state)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                            /* LLR-WDG-4 */
    }
    else
    {
        state->elapsed = 0u;                             /* LLR-WDG-5 */
        state->tripped = false;
        status = SC_OK;
    }

    return status;
}

void sc_watchdog_kick(sc_watchdog_state_t *state)
{
    if (state != NULL)                                   /* LLR-WDG-6 */
    {
        state->elapsed = 0u;                             /* LLR-WDG-7: tripped left as-is */
    }
}

sc_watchdog_verdict_t sc_watchdog_tick(const sc_watchdog_config_t *cfg,
                                       sc_watchdog_state_t *state)
{
    sc_watchdog_verdict_t verdict;

    if ((cfg == NULL) || (state == NULL))
    {
        verdict = SC_WATCHDOG_TRIPPED;                   /* LLR-WDG-8 */
    }
    else
    {
        if (state->elapsed < cfg->timeout)              /* LLR-WDG-9 */
        {
            state->elapsed = (uint16_t)(state->elapsed + 1u);
            if (state->elapsed >= cfg->timeout)          /* LLR-WDG-10: set, never cleared here */
            {
                state->tripped = true;
            }
        }

        verdict = state->tripped                         /* LLR-WDG-11 */
                ? SC_WATCHDOG_TRIPPED
                : SC_WATCHDOG_OK;
    }

    return verdict;
}

bool sc_watchdog_expired(const sc_watchdog_state_t *state)
{
    return (state != NULL) && state->tripped;            /* LLR-WDG-12 */
}
