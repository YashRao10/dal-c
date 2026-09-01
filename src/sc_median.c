/*
 * sc_median.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_median.md.
 *
 * The copy and sort loops are bounded by the compile-time array size
 * *and* the live sample count, so the scratch index is provably within
 * SC_MEDIAN_MAX_WINDOW regardless of the (validated) cfg->length.
 */
#include "sc_median.h"

sc_status_t sc_median_config_valid(const sc_median_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                            /* LLR-MEDIAN-3 */
    }
    else if ((cfg->length > SC_MEDIAN_MAX_WINDOW) ||
             ((cfg->length % 2u) == 0u))                 /* LLR-MEDIAN-4, LLR-MEDIAN-5 */
    {
        status = SC_ERR_PARAM;
    }
    else
    {
        status = SC_OK;                                  /* LLR-MEDIAN-6 */
    }

    return status;
}

sc_status_t sc_median_init(sc_median_state_t *state)
{
    sc_status_t status;

    if (state == NULL)
    {
        status = SC_ERR_NULL;                            /* LLR-MEDIAN-2 */
    }
    else
    {
        uint8_t i;
        for (i = 0u; i < SC_MEDIAN_MAX_WINDOW; i++)      /* LLR-MEDIAN-1 */
        {
            state->history[i] = 0;
        }
        state->count = 0u;
        state->head  = 0u;
        status = SC_OK;
    }

    return status;
}

int32_t sc_median_update(const sc_median_config_t *cfg,
                         sc_median_state_t *state,
                         int32_t sample)
{
    int32_t result;

    if ((cfg == NULL) || (state == NULL))
    {
        result = 0;                                      /* LLR-MEDIAN-7 */
    }
    else
    {
        int32_t sorted[SC_MEDIAN_MAX_WINDOW];
        uint8_t n;
        uint8_t i;

        state->history[state->head] = sample;            /* LLR-MEDIAN-8 */
        state->head = (uint8_t)((state->head + 1u) % cfg->length);

        if (state->count < cfg->length)                  /* LLR-MEDIAN-9 */
        {
            state->count = (uint8_t)(state->count + 1u);
        }

        n = state->count;
        for (i = 0u; (i < SC_MEDIAN_MAX_WINDOW) && (i < n); i++)  /* LLR-MEDIAN-10: copy */
        {
            sorted[i] = state->history[i];
        }
        for (i = 1u; (i < SC_MEDIAN_MAX_WINDOW) && (i < n); i++)  /* LLR-MEDIAN-10: insertion sort */
        {
            int32_t key = sorted[i];
            uint8_t j   = i;
            while ((j > 0u) && (sorted[j - 1u] > key))
            {
                sorted[j] = sorted[j - 1u];
                j--;
            }
            sorted[j] = key;
        }

        result = (n < SC_MEDIAN_MAX_WINDOW)              /* LLR-MEDIAN-11 */
               ? sorted[n / 2u]
               : sorted[SC_MEDIAN_MAX_WINDOW / 2u];
    }

    return result;
}
