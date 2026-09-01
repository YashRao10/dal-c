/*
 * test_sc_lut.c -- requirements-based tests for sc_lut.
 */
#include "sc_test.h"
#include "sc_suites.h"
#include "sc_lut.h"

/* x: 0 10 20 40   y: 0 100 100 300  (flat middle segment, steeper end) */
static const int32_t TX[4] = { 0, 10, 20, 40 };
static const int32_t TY[4] = { 0, 100, 100, 300 };
static const sc_lut_t T = { TX, TY, 4u };

/* HLR-LUT-3 / LLR-LUT-8,9,10 -- validation. */
static void lut_valid(void)
{
    static const int32_t bad_x[3] = { 0, 0, 1 };   /* not strictly increasing */
    static const int32_t y3[3]    = { 0, 1, 2 };
    sc_lut_t no_x = { NULL, TY, 4u };
    sc_lut_t no_y = { TX, NULL, 4u };
    sc_lut_t two = { TX, TY, 1u };
    sc_lut_t rev = { bad_x, y3, 3u };

    SC_CHECK_EQ(sc_lut_valid(NULL), SC_ERR_NULL);
    SC_CHECK_EQ(sc_lut_valid(&no_x), SC_ERR_NULL);
    SC_CHECK_EQ(sc_lut_valid(&no_y), SC_ERR_NULL);
    SC_CHECK_EQ(sc_lut_valid(&two), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_lut_valid(&rev), SC_ERR_PARAM);
    SC_CHECK_EQ(sc_lut_valid(&T), SC_OK);
}

/* HLR-LUT-1 / LLR-LUT-1 -- below the first breakpoint clamps to y[0]. */
static void lut_clamp_low(void)
{
    SC_CHECK_EQ(sc_lut_eval(&T, -100), 0);
    SC_CHECK_EQ(sc_lut_eval(&T, 0), 0);
}

/* HLR-LUT-1 / LLR-LUT-2 -- at or above the last breakpoint clamps to y[n-1]. */
static void lut_clamp_high(void)
{
    SC_CHECK_EQ(sc_lut_eval(&T, 40), 300);
    SC_CHECK_EQ(sc_lut_eval(&T, 1000), 300);
}

/* HLR-LUT-2 / LLR-LUT-3,4,5,6 -- linear interpolation within a segment. */
static void lut_interpolates(void)
{
    SC_CHECK_EQ(sc_lut_eval(&T, 5), 50);      /* midway on [0,10] -> [0,100]  */
    SC_CHECK_EQ(sc_lut_eval(&T, 10), 100);    /* exactly on a breakpoint      */
    SC_CHECK_EQ(sc_lut_eval(&T, 15), 100);    /* flat segment [10,20]         */
    SC_CHECK_EQ(sc_lut_eval(&T, 30), 200);    /* midway on [20,40] -> [100,300]*/
    SC_CHECK_EQ(sc_lut_eval(&T, 39), 290);
}

/* HLR-LUT-2 -- the segment search advances past several breakpoints. */
static void lut_segment_search(void)
{
    static const int32_t x8[5] = { 0, 100, 200, 300, 400 };
    static const int32_t y8[5] = { 0, 0, 0, 0, 800 };
    static const sc_lut_t big = { x8, y8, 5u };
    SC_CHECK_EQ(sc_lut_eval(&big, 350), 400);   /* segment [300,400] */
    SC_CHECK_EQ(sc_lut_eval(&big, 150), 0);
}

/* HLR-LUT-1 -- a NULL table evaluates to 0; a 2-point table works. */
static void lut_edges(void)
{
    static const int32_t x2[2] = { -10, 10 };
    static const int32_t y2[2] = { -1000, 1000 };
    static const sc_lut_t two = { x2, y2, 2u };
    SC_CHECK_EQ(sc_lut_eval(NULL, 5), 0);
    SC_CHECK_EQ(sc_lut_eval(&two, 0), 0);
    SC_CHECK_EQ(sc_lut_eval(&two, 5), 500);
}

const sc_test_case sc_suite_lut[] = {
    { "lut_valid",           lut_valid           },
    { "lut_clamp_low",       lut_clamp_low       },
    { "lut_clamp_high",      lut_clamp_high      },
    { "lut_interpolates",    lut_interpolates    },
    { "lut_segment_search",  lut_segment_search  },
    { "lut_edges",           lut_edges           }
};
const unsigned sc_suite_lut_count =
    (unsigned)(sizeof(sc_suite_lut) / sizeof(sc_suite_lut[0]));
