/*
 * test_sc_fixed.c -- requirements-based tests for sc_fixed (Q16.16).
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_fixed.h"

#define ONE   SC_Q16_ONE
#define HALF  ((sc_q16_t)0x8000)          /* 0.5 */
#define TWO   ((sc_q16_t)0x20000)         /* 2.0 */

/* HLR-FX-1 / LLR-FX-1,2,3 -- whole-number conversion, saturating. */
static void fx_from_int(void)
{
    SC_CHECK_EQ(sc_q16_from_int(0), 0);
    SC_CHECK_EQ(sc_q16_from_int(1), ONE);
    SC_CHECK_EQ(sc_q16_from_int(-3), (sc_q16_t)(-3 * ONE));
    SC_CHECK_EQ(sc_q16_from_int(32767), (sc_q16_t)(32767 * ONE));
    SC_CHECK_EQ(sc_q16_from_int(-32768), SC_Q16_MIN);
    SC_CHECK_EQ(sc_q16_from_int(40000), SC_Q16_MAX);
    SC_CHECK_EQ(sc_q16_from_int(-40000), SC_Q16_MIN);
}

/* HLR-FX-1 / LLR-FX-4 -- to_int truncates toward zero. */
static void fx_to_int(void)
{
    SC_CHECK_EQ(sc_q16_to_int(ONE), 1);
    SC_CHECK_EQ(sc_q16_to_int(ONE + HALF), 1);          /* 1.5 -> 1 */
    SC_CHECK_EQ(sc_q16_to_int(-(ONE + HALF)), -1);      /* -1.5 -> -1 */
    SC_CHECK_EQ(sc_q16_to_int(HALF), 0);
}

/* HLR-FX-2 / LLR-FX-5,6 -- add and subtract saturate. */
static void fx_add_sub(void)
{
    SC_CHECK_EQ(sc_q16_add(ONE, ONE), TWO);
    SC_CHECK_EQ(sc_q16_sub(ONE, TWO), -ONE);
    SC_CHECK_EQ(sc_q16_add(SC_Q16_MAX, ONE), SC_Q16_MAX);
    SC_CHECK_EQ(sc_q16_sub(SC_Q16_MIN, ONE), SC_Q16_MIN);
}

/* HLR-FX-3 / LLR-FX-7,8,9,13,14,15 -- multiply: rounding both signs,
 * saturation both ends, and the exact path. */
static void fx_mul(void)
{
    SC_CHECK_EQ(sc_q16_mul(TWO, ONE + HALF), 3 * ONE);          /* 2 * 1.5 = 3 */
    SC_CHECK_EQ(sc_q16_mul(-TWO, ONE + HALF), -3 * ONE);        /* -3 */
    SC_CHECK_EQ(sc_q16_mul(HALF, HALF), (sc_q16_t)0x4000);      /* 0.25 */
    /* rounding: (1/65536) * (1/65536) rounds to 0; 0.5 LSB rounds up */
    SC_CHECK_EQ(sc_q16_mul(1, 1), 0);
    SC_CHECK_EQ(sc_q16_mul(SC_Q16_MAX, TWO), SC_Q16_MAX);       /* saturate hi */
    SC_CHECK_EQ(sc_q16_mul(SC_Q16_MIN, TWO), SC_Q16_MIN);       /* saturate lo */
    SC_CHECK_EQ(sc_q16_mul(0, SC_Q16_MAX), 0);
}

/* HLR-FX-4 / LLR-FX-10,11 -- divide, including by zero. */
static void fx_div(void)
{
    SC_CHECK_EQ(sc_q16_div(3 * ONE, TWO), ONE + HALF);          /* 1.5 */
    SC_CHECK_EQ(sc_q16_div(-ONE, TWO), -HALF);
    SC_CHECK_EQ(sc_q16_div(5 * ONE, 0), SC_Q16_MAX);            /* +/0 */
    SC_CHECK_EQ(sc_q16_div(-5 * ONE, 0), SC_Q16_MIN);           /* -/0 */
    SC_CHECK_EQ(sc_q16_div(0, 0), SC_Q16_MAX);                  /* 0/0 */
    SC_CHECK_EQ(sc_q16_div(SC_Q16_MAX, HALF), SC_Q16_MAX);      /* saturates */
    SC_CHECK_EQ(sc_q16_div(SC_Q16_MIN, HALF), SC_Q16_MIN);
}

/* HLR-FX-5 / LLR-FX-16,17,18 -- absolute value, saturating. */
static void fx_abs(void)
{
    SC_CHECK_EQ(sc_q16_abs(ONE), ONE);
    SC_CHECK_EQ(sc_q16_abs(-ONE), ONE);
    SC_CHECK_EQ(sc_q16_abs(0), 0);
    SC_CHECK_EQ(sc_q16_abs(SC_Q16_MIN), SC_Q16_MAX);
}

/* HLR-FX-6 / LLR-FX-12,19,20,21 -- clamp. */
static void fx_clamp(void)
{
    SC_CHECK_EQ(sc_q16_clamp(5 * ONE, 0, 10 * ONE), 5 * ONE);
    SC_CHECK_EQ(sc_q16_clamp(-ONE, 0, 10 * ONE), 0);
    SC_CHECK_EQ(sc_q16_clamp(20 * ONE, 0, 10 * ONE), 10 * ONE);
    SC_CHECK_EQ(sc_q16_clamp(5 * ONE, 10 * ONE, 0), 10 * ONE);  /* lo > hi */
}

const sc_test_case sc_suite_fixed[] = {
    { "fx_from_int", fx_from_int },
    { "fx_to_int",   fx_to_int   },
    { "fx_add_sub",  fx_add_sub  },
    { "fx_mul",      fx_mul      },
    { "fx_div",      fx_div      },
    { "fx_abs",      fx_abs      },
    { "fx_clamp",    fx_clamp    }
};
const unsigned sc_suite_fixed_count =
    (unsigned)(sizeof(sc_suite_fixed) / sizeof(sc_suite_fixed[0]));
