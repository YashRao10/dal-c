/*
 * test_sc_median.c -- requirements-based tests for sc_median.
 */
#include <stdint.h>

#include "sc_test.h"
#include "sc_suites.h"
#include "sc_median.h"

static const sc_median_config_t CFG1 = { 1u };
static const sc_median_config_t CFG3 = { 3u };
static const sc_median_config_t CFG5 = { 5u };

/* HLR-MEDIAN-1 / LLR-MEDIAN-3,4,5,6 -- config validation. Each condition of
 * the compound decision is shown to independently decide the outcome. */
static void median_config_validation(void)
{
    static const sc_median_config_t len0  = { 0u };
    static const sc_median_config_t len2  = { 2u };
    static const sc_median_config_t len4  = { 4u };
    static const sc_median_config_t len15 = { 15u };
    static const sc_median_config_t len17 = { 17u };

    SC_CHECK_EQ(sc_median_config_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_median_config_valid(&len0), SC_ERR_PARAM);   /* even (0)          */
    SC_CHECK_EQ(sc_median_config_valid(&len2), SC_ERR_PARAM);   /* even              */
    SC_CHECK_EQ(sc_median_config_valid(&len4), SC_ERR_PARAM);   /* in range, even    */
    SC_CHECK_EQ(sc_median_config_valid(&len17), SC_ERR_PARAM);  /* odd, over the max */
    SC_CHECK_EQ(sc_median_config_valid(&CFG1), SC_OK);
    SC_CHECK_EQ(sc_median_config_valid(&CFG3), SC_OK);
    SC_CHECK_EQ(sc_median_config_valid(&len15), SC_OK);         /* boundary          */
}

/* HLR-MEDIAN-2 / LLR-MEDIAN-1,2,7 -- init to a known state, null-safety. */
static void median_init_and_null(void)
{
    sc_median_state_t st;
    unsigned i;

    SC_CHECK_EQ(sc_median_init(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_median_init(&st), SC_OK);
    SC_CHECK_EQ(st.count, 0u);
    SC_CHECK_EQ(st.head, 0u);
    for (i = 0u; i < SC_MEDIAN_MAX_WINDOW; i++)
    {
        SC_CHECK(st.history[i] == 0);
    }

    /* NULL cfg or state: return 0, leave the state untouched. */
    SC_CHECK(sc_median_update(NULL, &st, 5) == 0);
    SC_CHECK_EQ(st.count, 0u);
    SC_CHECK_EQ(st.head, 0u);
    SC_CHECK(sc_median_update(&CFG3, NULL, 5) == 0);
}

/* HLR-MEDIAN-4 -- a window of one is a passthrough. Also drives the
 * count-saturation decision to false on the second update. */
static void median_window_of_one(void)
{
    sc_median_state_t st;
    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG1, &st, 42) == 42);
    SC_CHECK_EQ(st.count, 1u);
    SC_CHECK(sc_median_update(&CFG1, &st, -7) == -7);
    SC_CHECK_EQ(st.count, 1u);          /* held, not incremented past length */
}

/* HLR-MEDIAN-4 -- before the window fills, the median is taken over the
 * samples seen so far (LLR-MEDIAN-11: index count/2). */
static void median_partial_fill(void)
{
    sc_median_state_t st;
    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG3, &st, 5) == 5);   /* count 1: {5}        */
    SC_CHECK(sc_median_update(&CFG3, &st, 9) == 9);   /* count 2: {5,9}[1]=9 */
    SC_CHECK(sc_median_update(&CFG3, &st, 1) == 5);   /* count 3: {1,5,9}[1] */
}

/* HLR-MEDIAN-3 / HLR-MEDIAN-4 -- an isolated spike never reaches the
 * output once the window holds three samples. */
static void median_rejects_spike(void)
{
    sc_median_state_t st;
    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG3, &st, 10) == 10);
    SC_CHECK(sc_median_update(&CFG3, &st, 10) == 10);
    SC_CHECK(sc_median_update(&CFG3, &st, 10) == 10);
    SC_CHECK(sc_median_update(&CFG3, &st, 999) == 10);   /* positive spike */
    SC_CHECK(sc_median_update(&CFG3, &st, 10) == 10);
    SC_CHECK(sc_median_update(&CFG3, &st, -999) == 10);  /* negative spike */
    SC_CHECK(sc_median_update(&CFG3, &st, 10) == 10);
    SC_CHECK(sc_median_update(&CFG3, &st, 10) == 10);
}

/* HLR-MEDIAN-3 -- the window slides: the oldest sample is overwritten
 * once `length` samples have been seen (LLR-MEDIAN-8 head wrap). */
static void median_slides(void)
{
    sc_median_state_t st;
    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG3, &st, 1) == 1);
    SC_CHECK(sc_median_update(&CFG3, &st, 2) == 2);
    SC_CHECK(sc_median_update(&CFG3, &st, 3) == 2);   /* {1,2,3}          */
    SC_CHECK(sc_median_update(&CFG3, &st, 4) == 3);   /* {2,3,4}, 1 gone  */
    SC_CHECK(sc_median_update(&CFG3, &st, 5) == 4);   /* {3,4,5}          */
    SC_CHECK(sc_median_update(&CFG3, &st, 100) == 5); /* {4,5,100}        */
    SC_CHECK(sc_median_update(&CFG3, &st, 6) == 6);   /* {5,100,6}        */
    SC_CHECK_EQ(st.count, 3u);
}

/* LLR-MEDIAN-10 -- exercises every branch of the insertion-sort inner
 * decision `(j > 0) && (sorted[j-1] > key)`:
 *   - both true            (a larger element is shifted right)
 *   - j > 0, compare false (the key sits mid-array)
 *   - j == 0               (the key is the new minimum)
 */
static void median_sort_paths(void)
{
    sc_median_state_t st;
    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG3, &st, 30) == 30);       /* {30}            */
    SC_CHECK(sc_median_update(&CFG3, &st, 20) == 30);       /* key 20 -> front */
    SC_CHECK(sc_median_update(&CFG3, &st, 40) == 30);       /* key 40 -> end   */

    /* fully descending input: every insertion shifts to the front */
    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG5, &st, 50) == 50);
    SC_CHECK(sc_median_update(&CFG5, &st, 40) == 50);
    SC_CHECK(sc_median_update(&CFG5, &st, 30) == 40);
    SC_CHECK(sc_median_update(&CFG5, &st, 20) == 40);
    SC_CHECK(sc_median_update(&CFG5, &st, 10) == 30);       /* {10,20,30,40,50}*/
}

/* LLR-MEDIAN-9 -- with a full window the count stays pinned at `length`
 * across many updates; extreme values sort without overflow. */
static void median_full_window_steady(void)
{
    sc_median_state_t st;
    unsigned k;

    SC_MUST(sc_median_init(&st));
    SC_CHECK(sc_median_update(&CFG5, &st, 5) == 5);
    SC_CHECK(sc_median_update(&CFG5, &st, 4) == 5);
    SC_CHECK(sc_median_update(&CFG5, &st, 3) == 4);
    SC_CHECK(sc_median_update(&CFG5, &st, 2) == 4);
    SC_CHECK(sc_median_update(&CFG5, &st, 1) == 3);   /* {1,2,3,4,5}   */
    SC_CHECK(sc_median_update(&CFG5, &st, 0) == 2);   /* {0,1,2,3,4}   */
    SC_CHECK(sc_median_update(&CFG5, &st, 0) == 1);   /* {0,0,1,2,3}   */
    for (k = 0u; k < 6u; k++)
    {
        (void)sc_median_update(&CFG5, &st, 0);
        SC_CHECK_EQ(st.count, 5u);
    }
    SC_CHECK(sc_median_update(&CFG5, &st, 0) == 0);   /* {0,0,0,0,0}   */

    SC_MUST(sc_median_init(&st));
    (void)sc_median_update(&CFG3, &st, INT32_MIN);
    (void)sc_median_update(&CFG3, &st, INT32_MAX);
    SC_CHECK(sc_median_update(&CFG3, &st, 0) == 0);   /* {MIN,0,MAX}[1]=0 */
}

/* HLR-MEDIAN-3 / HLR-MEDIAN-4 -- a full window at the maximum width:
 * exercises the copy/sort loop bound reaching SC_MEDIAN_MAX_WINDOW and the
 * count == SC_MEDIAN_MAX_WINDOW branch of the result index. */
static void median_max_window(void)
{
    static const sc_median_config_t cfg15 = { 15u };
    sc_median_state_t st;
    int32_t v = 0;
    unsigned k;

    SC_MUST(sc_median_init(&st));
    for (k = 1u; k <= 15u; k++)
    {
        v = sc_median_update(&cfg15, &st, (int32_t)k);
    }
    SC_CHECK(v == 8);                    /* window {1..15}, median is 8 */
    SC_CHECK_EQ(st.count, 15u);

    /* an isolated spike still cannot reach the output of a full window */
    SC_CHECK(sc_median_update(&cfg15, &st, 100000) != 100000);
    SC_CHECK(sc_median_update(&cfg15, &st, -100000) != -100000);

    /* a full window of one value returns that value */
    SC_MUST(sc_median_init(&st));
    for (k = 0u; k < 20u; k++)
    {
        v = sc_median_update(&cfg15, &st, 42);
    }
    SC_CHECK(v == 42);
    SC_CHECK_EQ(st.count, 15u);
}

const sc_test_case sc_suite_median[] = {
    { "median_config_validation",   median_config_validation   },
    { "median_init_and_null",       median_init_and_null       },
    { "median_window_of_one",       median_window_of_one       },
    { "median_partial_fill",        median_partial_fill        },
    { "median_rejects_spike",       median_rejects_spike       },
    { "median_slides",              median_slides              },
    { "median_sort_paths",          median_sort_paths          },
    { "median_full_window_steady",  median_full_window_steady  },
    { "median_max_window",          median_max_window          }
};
const unsigned sc_suite_median_count =
    (unsigned)(sizeof(sc_suite_median) / sizeof(sc_suite_median[0]));
