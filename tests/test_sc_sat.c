/*
 * test_sc_sat.c -- requirements-based tests for sc_sat.
 *
 * Cases are chosen to give MC/DC over the two compound decisions in each
 * function (see requirements/sc_sat.md).
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_sat.h"

/* HLR-SAT-1 / LLR-SAT-3 -- add returns the true sum when it fits. */
static void sat_add_normal(void)
{
    SC_CHECK_EQ(sc_sat_add_i32(10, 20), 30);
    SC_CHECK_EQ(sc_sat_add_i32(-5, 3), -2);
    SC_CHECK_EQ(sc_sat_add_i32(0, 0), 0);
    SC_CHECK_EQ(sc_sat_add_i32(INT32_MAX, INT32_MIN), -1);
}

/* HLR-SAT-2 / LLR-SAT-1 -- add pins to INT32_MAX on positive overflow.
 * Decision 1: both conditions true. */
static void sat_add_overflow_high(void)
{
    SC_CHECK_EQ(sc_sat_add_i32(INT32_MAX, 1), INT32_MAX);
    SC_CHECK_EQ(sc_sat_add_i32(INT32_MAX, INT32_MAX), INT32_MAX);
    SC_CHECK_EQ(sc_sat_add_i32(2000000000, 2000000000), INT32_MAX);
}

/* HLR-SAT-2 / LLR-SAT-2 -- add pins to INT32_MIN on negative overflow.
 * Decision 2: both conditions true. */
static void sat_add_overflow_low(void)
{
    SC_CHECK_EQ(sc_sat_add_i32(INT32_MIN, -1), INT32_MIN);
    SC_CHECK_EQ(sc_sat_add_i32(INT32_MIN, INT32_MIN), INT32_MIN);
}

/* Decision-1 second condition FALSE with first TRUE (b > 0, no overflow);
 * decision-2 second condition FALSE with first TRUE (b < 0, no overflow). */
static void sat_add_positive_and_negative_addend_no_overflow(void)
{
    SC_CHECK_EQ(sc_sat_add_i32(0, 1), 1);     /* b>0, a not near the top   */
    SC_CHECK_EQ(sc_sat_add_i32(0, -1), -1);   /* b<0, a not near the floor */
}

/* HLR-SAT-1 / LLR-SAT-6 -- subtract returns the true difference when it fits. */
static void sat_sub_normal(void)
{
    SC_CHECK_EQ(sc_sat_sub_i32(20, 5), 15);
    SC_CHECK_EQ(sc_sat_sub_i32(-2, 3), -5);
    SC_CHECK_EQ(sc_sat_sub_i32(INT32_MIN, INT32_MIN), 0);
}

/* HLR-SAT-2 / LLR-SAT-4 -- subtract pins to INT32_MAX on positive overflow. */
static void sat_sub_overflow_high(void)
{
    SC_CHECK_EQ(sc_sat_sub_i32(INT32_MAX, -1), INT32_MAX);
    SC_CHECK_EQ(sc_sat_sub_i32(INT32_MAX, INT32_MIN), INT32_MAX);
}

/* HLR-SAT-2 / LLR-SAT-5 -- subtract pins to INT32_MIN on negative overflow. */
static void sat_sub_overflow_low(void)
{
    SC_CHECK_EQ(sc_sat_sub_i32(INT32_MIN, 1), INT32_MIN);
    SC_CHECK_EQ(sc_sat_sub_i32(INT32_MIN, INT32_MAX), INT32_MIN);
}

/* Subtract: first condition TRUE, second FALSE, on each decision. */
static void sat_sub_near_miss(void)
{
    SC_CHECK_EQ(sc_sat_sub_i32(0, -1), 1);   /* b<0, a well below the top   */
    SC_CHECK_EQ(sc_sat_sub_i32(0, 1), -1);   /* b>0, a well above the floor */
}

const sc_test_case sc_suite_sat[] = {
    { "sat_add_normal",         sat_add_normal         },
    { "sat_add_overflow_high",  sat_add_overflow_high  },
    { "sat_add_overflow_low",   sat_add_overflow_low   },
    { "sat_add_pos_neg_addend_no_overflow",
      sat_add_positive_and_negative_addend_no_overflow },
    { "sat_sub_normal",         sat_sub_normal         },
    { "sat_sub_overflow_high",  sat_sub_overflow_high  },
    { "sat_sub_overflow_low",   sat_sub_overflow_low   },
    { "sat_sub_near_miss",      sat_sub_near_miss      }
};
const unsigned sc_suite_sat_count =
    (unsigned)(sizeof(sc_suite_sat) / sizeof(sc_suite_sat[0]));
