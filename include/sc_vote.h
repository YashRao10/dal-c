/*
 * sc_vote.h -- redundancy voter for replicated channel readings.
 *
 * Given several independent readings of one quantity, decide which value to
 * believe. Readings are grouped into agreement clusters (two readings agree
 * when |a - b| <= tolerance); the value backed by the largest cluster wins.
 * The result also carries the size of that cluster, a bitmask of the
 * channels that disagreed with the winning value, and a verdict saying
 * whether enough channels agreed to trust the result.
 *
 * A dual/triple/quad sensor set feeds sc_vote each frame; the caller uses
 * `verdict` to decide whether to act on `value` and `dissenting` to latch
 * or annunciate a failed channel.
 *
 * Stateless and allocation-free. Working storage is sized by
 * SC_VOTE_MAX_INPUTS and every loop is bounded by it.
 */
#ifndef DAL_C_SC_VOTE_H
#define DAL_C_SC_VOTE_H

#include "sc_common.h"

/* Largest channel set the voter accepts; sizes the internal work arrays and
 * the width of the `dissenting` mask. */
#define SC_VOTE_MAX_INPUTS 8

typedef enum
{
    SC_VOTE_OK           = 0,  /* winning cluster held at least `agree` channels */
    SC_VOTE_NO_CONSENSUS = 1   /* the largest cluster was smaller than `agree`   */
} sc_vote_verdict_t;

typedef struct
{
    uint8_t count;      /* number of channels supplied, 2 .. SC_VOTE_MAX_INPUTS  */
    uint8_t agree;      /* channels that must cluster for SC_VOTE_OK, 1 .. count  */
    int32_t tolerance;  /* two readings agree when |a - b| <= tolerance, >= 0    */
} sc_vote_config_t;

typedef struct
{
    int32_t           value;      /* voted reading: the winning channel's input  */
    uint8_t           agreeing;   /* size of the winning cluster, 1 .. count     */
    uint8_t           dissenting; /* bit i set: channel i disagrees with `value` */
    sc_vote_verdict_t verdict;    /* SC_VOTE_OK or SC_VOTE_NO_CONSENSUS          */
} sc_vote_result_t;

/*
 * Validate a configuration.
 *   SC_OK        usable
 *   SC_ERR_NULL  cfg is NULL
 *   SC_ERR_PARAM count outside 2..SC_VOTE_MAX_INPUTS, agree outside 1..count,
 *                or tolerance negative
 */
SC_NODISCARD sc_status_t sc_vote_config_valid(const sc_vote_config_t *cfg);

/*
 * Vote on `n` readings in `inputs` and write the outcome to `*out`.
 *
 * Preconditions: `cfg` has passed sc_vote_config_valid(); `inputs` points to
 * at least `n` readings. `n` must equal `cfg->count`.
 *
 *   SC_OK         *out fully populated (check out->verdict for the outcome)
 *   SC_ERR_NULL   cfg, inputs, or out is NULL -- *out untouched
 *   SC_ERR_PARAM  n != cfg->count -- *out untouched
 */
SC_NODISCARD sc_status_t sc_vote_evaluate(const sc_vote_config_t *cfg,
                                          const int32_t *inputs, uint8_t n,
                                          sc_vote_result_t *out);

#endif /* DAL_C_SC_VOTE_H */
