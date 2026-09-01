/*
 * sc_vote.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_vote.md.
 *
 * The cluster scan is O(n^2) over at most SC_VOTE_MAX_INPUTS channels, with
 * both loop variables bounded by the compile-time array size *and* the
 * validated channel count, so every index is provably in range.
 */
#include "sc_vote.h"

/* LLR-VOTE-9: |a - b| <= tol, computed in 64-bit so no int32_t pair can
 * overflow the subtraction. */
static bool within(int32_t a, int32_t b, int32_t tolerance)
{
    int64_t diff = (int64_t)a - (int64_t)b;

    if (diff < 0)
    {
        diff = -diff;
    }

    return (diff <= (int64_t)tolerance);
}

sc_status_t sc_vote_config_valid(const sc_vote_config_t *cfg)
{
    sc_status_t status;

    if (cfg == NULL)
    {
        status = SC_ERR_NULL;                                    /* LLR-VOTE-1 */
    }
    else if ((cfg->count < 2) || (cfg->count > SC_VOTE_MAX_INPUTS))
    {
        status = SC_ERR_PARAM;                                   /* LLR-VOTE-2 */
    }
    else if ((cfg->agree == 0u) || (cfg->agree > cfg->count))
    {
        status = SC_ERR_PARAM;                                   /* LLR-VOTE-3 */
    }
    else if (cfg->tolerance < 0)
    {
        status = SC_ERR_PARAM;                                   /* LLR-VOTE-4 */
    }
    else
    {
        status = SC_OK;                                          /* LLR-VOTE-5 */
    }

    return status;
}

sc_status_t sc_vote_evaluate(const sc_vote_config_t *cfg,
                             const int32_t *inputs, uint8_t n,
                             sc_vote_result_t *out)
{
    sc_status_t status;

    if ((cfg == NULL) || (inputs == NULL) || (out == NULL))
    {
        status = SC_ERR_NULL;                                    /* LLR-VOTE-6 */
    }
    else if (n != cfg->count)
    {
        status = SC_ERR_PARAM;                                   /* LLR-VOTE-7 */
    }
    else
    {
        uint8_t sizes[SC_VOTE_MAX_INPUTS] = { 0u };
        uint8_t winner = 0u;
        uint8_t mask   = 0u;
        uint8_t i;
        uint8_t j;

        for (i = 0u; (i < SC_VOTE_MAX_INPUTS) && (i < n); i++)   /* LLR-VOTE-8 */
        {
            uint8_t cluster = 0u;
            for (j = 0u; (j < SC_VOTE_MAX_INPUTS) && (j < n); j++)
            {
                if (within(inputs[i], inputs[j], cfg->tolerance))
                {
                    cluster = (uint8_t)(cluster + 1u);
                }
            }
            sizes[i] = cluster;

            if (sizes[i] > sizes[winner])                        /* LLR-VOTE-10 */
            {
                winner = i;
            }
        }

        out->value    = inputs[winner];                          /* LLR-VOTE-11 */
        out->agreeing = sizes[winner];

        for (i = 0u; (i < SC_VOTE_MAX_INPUTS) && (i < n); i++)   /* LLR-VOTE-13 */
        {
            if (!within(inputs[i], out->value, cfg->tolerance))
            {
                mask = (uint8_t)(mask | (uint8_t)(1u << i));
            }
        }
        out->dissenting = mask;

        out->verdict = (out->agreeing >= cfg->agree)             /* LLR-VOTE-12 */
                     ? SC_VOTE_OK
                     : SC_VOTE_NO_CONSENSUS;

        status = SC_OK;                                          /* LLR-VOTE-14 */
    }

    return status;
}
