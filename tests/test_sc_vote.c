/*
 * test_sc_vote.c -- requirements-based tests for sc_vote.
 */
#include <stdint.h>

#include "sc_test.h"
#include "sc_suites.h"
#include "sc_vote.h"

/* HLR-VOTE-1 / LLR-VOTE-1,2,3,4,5 -- config validation. Each condition of
 * every compound decision is shown to independently decide the outcome. */
static void vote_config_validation(void)
{
    sc_vote_config_t c;

    SC_CHECK_EQ(sc_vote_config_valid(NULL), SC_ERR_NULL);

    /* count: (< 2) || (> MAX) */
    c.agree = 1u; c.tolerance = 0;
    c.count = 1u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_ERR_PARAM);  /* count<2 decides  */
    c.count = 9u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_ERR_PARAM);  /* count>MAX decides*/
    c.count = 2u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_OK);         /* both false       */
    c.count = 8u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_OK);         /* MAX boundary     */

    /* agree: (== 0) || (> count) */
    c.count = 3u; c.tolerance = 0;
    c.agree = 0u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_ERR_PARAM);  /* agree==0 decides */
    c.agree = 4u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_ERR_PARAM);  /* agree>count deci */
    c.agree = 2u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_OK);         /* both false       */
    c.agree = 3u;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_OK);         /* agree==count ok  */

    /* tolerance: (< 0) */
    c.count = 3u; c.agree = 2u;
    c.tolerance = -1;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_ERR_PARAM);
    c.tolerance =  0;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_OK);
    c.tolerance = 25;  SC_CHECK_EQ(sc_vote_config_valid(&c), SC_OK);
}

/* HLR-VOTE-2 / LLR-VOTE-6,7 -- null-safety and the n/count check. Each
 * operand of (cfg||inputs||out == NULL) is shown to independently decide. */
static void vote_null_and_arity(void)
{
    static const sc_vote_config_t c3 = { 3u, 2u, 5 };
    static const int32_t in[3] = { 10, 10, 10 };
    sc_vote_result_t r;

    SC_CHECK_EQ(sc_vote_evaluate(NULL, in, 3u, &r), SC_ERR_NULL);   /* cfg decides    */
    SC_CHECK_EQ(sc_vote_evaluate(&c3, NULL, 3u, &r), SC_ERR_NULL);  /* inputs decides */
    SC_CHECK_EQ(sc_vote_evaluate(&c3, in, 3u, NULL), SC_ERR_NULL);  /* out decides    */

    SC_CHECK_EQ(sc_vote_evaluate(&c3, in, 2u, &r), SC_ERR_PARAM);   /* n < count      */
    SC_CHECK_EQ(sc_vote_evaluate(&c3, in, 4u, &r), SC_ERR_PARAM);   /* n > count      */
    SC_CHECK_EQ(sc_vote_evaluate(&c3, in, 3u, &r), SC_OK);          /* n == count     */
}

/* HLR-VOTE-3 / HLR-VOTE-4 / LLR-VOTE-11,12,14 -- a clean unanimous vote. */
static void vote_unanimous(void)
{
    static const sc_vote_config_t c = { 3u, 3u, 0 };
    static const int32_t in[3] = { 50, 50, 50 };
    sc_vote_result_t r;

    SC_CHECK_EQ(sc_vote_evaluate(&c, in, 3u, &r), SC_OK);
    SC_CHECK_EQ(r.value, 50);
    SC_CHECK_EQ(r.agreeing, 3u);
    SC_CHECK_EQ(r.dissenting, 0u);
    SC_CHECK_EQ(r.verdict, SC_VOTE_OK);
}

/* HLR-VOTE-3 / LLR-VOTE-10 -- winner moves to a strictly larger cluster,
 * then stays put when later channels only tie it. LLR-VOTE-13 mask. */
static void vote_majority_with_outlier(void)
{
    static const sc_vote_config_t c = { 4u, 3u, 5 };
    static const int32_t in[4] = { 5, 100, 101, 102 };  /* ch0 is the outlier */
    sc_vote_result_t r;

    SC_CHECK_EQ(sc_vote_evaluate(&c, in, 4u, &r), SC_OK);
    SC_CHECK_EQ(r.value, 100);          /* winner is ch1; ch2/ch3 only tie it */
    SC_CHECK_EQ(r.agreeing, 3u);
    SC_CHECK_EQ(r.dissenting, 0x1u);    /* only ch0 disagrees                  */
    SC_CHECK_EQ(r.verdict, SC_VOTE_OK);
}

/* HLR-VOTE-3 / LLR-VOTE-10 -- every cluster the same size: winner stays at
 * the earliest channel (the `>` comparison is never true after i=0). */
static void vote_tie_keeps_earliest(void)
{
    static const sc_vote_config_t c = { 4u, 2u, 5 };
    static const int32_t in[4] = { 1, 1, 100, 100 };
    sc_vote_result_t r;

    SC_CHECK_EQ(sc_vote_evaluate(&c, in, 4u, &r), SC_OK);
    SC_CHECK_EQ(r.value, 1);            /* not 100, though its cluster is equal */
    SC_CHECK_EQ(r.agreeing, 2u);
    SC_CHECK_EQ(r.dissenting, 0xCu);    /* ch2, ch3 disagree with value 1       */
    SC_CHECK_EQ(r.verdict, SC_VOTE_OK);
}

/* HLR-VOTE-4 / LLR-VOTE-12,14 -- no cluster reaches `agree`: NO_CONSENSUS,
 * still SC_OK, `value` is the best guess. LLR-VOTE-9 -- extreme inputs must
 * not overflow the within-tolerance subtraction. */
static void vote_no_consensus_no_overflow(void)
{
    static const sc_vote_config_t c = { 3u, 2u, 0 };
    static const int32_t in[3] = { INT32_MIN, INT32_MAX, 0 };
    sc_vote_result_t r;

    SC_CHECK_EQ(sc_vote_evaluate(&c, in, 3u, &r), SC_OK);
    SC_CHECK_EQ(r.value, INT32_MIN);    /* first channel; every cluster is 1   */
    SC_CHECK_EQ(r.agreeing, 1u);
    SC_CHECK_EQ(r.dissenting, 0x6u);    /* ch1, ch2 disagree                   */
    SC_CHECK_EQ(r.verdict, SC_VOTE_NO_CONSENSUS);
}

/* LLR-VOTE-8,9 -- tolerance banding: readings within `tolerance` cluster,
 * those just outside do not. Drives within()'s diff<0 both ways and its
 * `<= tol` decision both ways. */
static void vote_tolerance_band(void)
{
    static const sc_vote_config_t c = { 3u, 2u, 10 };
    sc_vote_result_t r;

    {   /* {100, 108, 250}: ch0,ch1 within 10, ch2 far out */
        static const int32_t in[3] = { 100, 108, 250 };
        SC_CHECK_EQ(sc_vote_evaluate(&c, in, 3u, &r), SC_OK);
        SC_CHECK_EQ(r.value, 100);
        SC_CHECK_EQ(r.agreeing, 2u);
        SC_CHECK_EQ(r.dissenting, 0x4u);
        SC_CHECK_EQ(r.verdict, SC_VOTE_OK);
    }
    {   /* the mid reading is within tolerance of both neighbours, so it
         * clusters with the whole set (|100-110| and |110-111| each <= 10)
         * even though the outer two are 11 apart */
        static const int32_t in[3] = { 100, 110, 111 };
        SC_CHECK_EQ(sc_vote_evaluate(&c, in, 3u, &r), SC_OK);
        SC_CHECK_EQ(r.value, 110);          /* ch1 wins on the boundary case  */
        SC_CHECK_EQ(r.agreeing, 3u);
        SC_CHECK_EQ(r.dissenting, 0u);
        SC_CHECK_EQ(r.verdict, SC_VOTE_OK);
    }
    {   /* no mid reading: ch0 pairs with ch2 (|10| <= 10), ch1 is 11 out */
        static const int32_t in[3] = { 100, 111, 90 };
        SC_CHECK_EQ(sc_vote_evaluate(&c, in, 3u, &r), SC_OK);
        SC_CHECK_EQ(r.value, 100);
        SC_CHECK_EQ(r.agreeing, 2u);
        SC_CHECK_EQ(r.dissenting, 0x2u);    /* ch1 (|11| > 10) dissents        */
    }
}

/* HLR-VOTE-3 / HLR-VOTE-5 -- a full-width channel set: exercises the copy /
 * scan loop bound reaching SC_VOTE_MAX_INPUTS and mask bits in the high
 * half. */
static void vote_max_inputs(void)
{
    static const sc_vote_config_t c = { 8u, 5u, 5 };
    static const int32_t in[8] = { 10, 10, 10, 10, 10, 10, 999, 998 };
    sc_vote_result_t r;

    SC_CHECK_EQ(sc_vote_evaluate(&c, in, 8u, &r), SC_OK);
    SC_CHECK_EQ(r.value, 10);
    SC_CHECK_EQ(r.agreeing, 6u);
    SC_CHECK_EQ(r.dissenting, 0xC0u);       /* ch6, ch7 */
    SC_CHECK_EQ(r.verdict, SC_VOTE_OK);

    {   /* same width, split 4/4 with agree=5 -> no consensus */
        static const sc_vote_config_t c2 = { 8u, 5u, 3 };
        static const int32_t split[8] = { 0, 1, 2, 1, 500, 501, 500, 502 };
        SC_CHECK_EQ(sc_vote_evaluate(&c2, split, 8u, &r), SC_OK);
        SC_CHECK_EQ(r.agreeing, 4u);
        SC_CHECK_EQ(r.verdict, SC_VOTE_NO_CONSENSUS);
    }
}

/* HLR-VOTE-3 -- a dual channel that agrees, and one that splits (agree=2 is
 * unreachable, so a 1/1 split is NO_CONSENSUS with the earliest reading). */
static void vote_dual_channel(void)
{
    static const sc_vote_config_t c = { 2u, 2u, 4 };
    sc_vote_result_t r;

    {
        static const int32_t ok[2] = { -3, 1 };   /* |4| <= 4 */
        SC_CHECK_EQ(sc_vote_evaluate(&c, ok, 2u, &r), SC_OK);
        SC_CHECK_EQ(r.agreeing, 2u);
        SC_CHECK_EQ(r.verdict, SC_VOTE_OK);
        SC_CHECK_EQ(r.dissenting, 0u);
    }
    {
        static const int32_t bad[2] = { -3, 3 };  /* |6| > 4 */
        SC_CHECK_EQ(sc_vote_evaluate(&c, bad, 2u, &r), SC_OK);
        SC_CHECK_EQ(r.value, -3);
        SC_CHECK_EQ(r.agreeing, 1u);
        SC_CHECK_EQ(r.verdict, SC_VOTE_NO_CONSENSUS);
        SC_CHECK_EQ(r.dissenting, 0x2u);
    }
}

const sc_test_case sc_suite_vote[] = {
    { "vote_config_validation",      vote_config_validation      },
    { "vote_null_and_arity",         vote_null_and_arity         },
    { "vote_unanimous",              vote_unanimous              },
    { "vote_majority_with_outlier",  vote_majority_with_outlier  },
    { "vote_tie_keeps_earliest",     vote_tie_keeps_earliest     },
    { "vote_no_consensus_no_overflow", vote_no_consensus_no_overflow },
    { "vote_tolerance_band",         vote_tolerance_band         },
    { "vote_max_inputs",             vote_max_inputs             },
    { "vote_dual_channel",           vote_dual_channel           }
};
const unsigned sc_suite_vote_count =
    (unsigned)(sizeof(sc_suite_vote) / sizeof(sc_suite_vote[0]));
