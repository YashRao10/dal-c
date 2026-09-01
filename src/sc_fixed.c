/*
 * sc_fixed.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_fixed.md.
 */
#include "sc_fixed.h"
#include "sc_sat.h"

#define Q16_INT_MAX   ((int32_t)32767)
#define Q16_INT_MIN   ((int32_t)(-32768))
#define Q16_HALF      ((int64_t)0x8000)
#define Q16_SCALE     ((int64_t)0x10000)

static sc_q16_t clamp_i64(int64_t v)
{
    sc_q16_t result;

    if (v > (int64_t)SC_Q16_MAX)
    {
        result = SC_Q16_MAX;                         /* LLR-FX-13 */
    }
    else if (v < (int64_t)SC_Q16_MIN)
    {
        result = SC_Q16_MIN;                         /* LLR-FX-14 */
    }
    else
    {
        result = (sc_q16_t)v;                        /* LLR-FX-15 */
    }

    return result;
}

sc_q16_t sc_q16_from_int(int32_t value)
{
    sc_q16_t result;

    if (value > Q16_INT_MAX)
    {
        result = SC_Q16_MAX;                         /* LLR-FX-1 */
    }
    else if (value < Q16_INT_MIN)
    {
        result = SC_Q16_MIN;                         /* LLR-FX-2 */
    }
    else
    {
        result = (sc_q16_t)(value * SC_Q16_ONE);     /* LLR-FX-3 */
    }

    return result;
}

int32_t sc_q16_to_int(sc_q16_t q)
{
    return (int32_t)(q / SC_Q16_ONE);               /* LLR-FX-4 */
}

sc_q16_t sc_q16_add(sc_q16_t a, sc_q16_t b)
{
    return sc_sat_add_i32(a, b);                     /* LLR-FX-5 */
}

sc_q16_t sc_q16_sub(sc_q16_t a, sc_q16_t b)
{
    return sc_sat_sub_i32(a, b);                     /* LLR-FX-6 */
}

sc_q16_t sc_q16_mul(sc_q16_t a, sc_q16_t b)
{
    int64_t p = (int64_t)a * (int64_t)b;

    if (p >= 0)
    {
        p += Q16_HALF;                               /* LLR-FX-7: round */
    }
    else
    {
        p -= Q16_HALF;                               /* LLR-FX-8: round away */
    }

    return clamp_i64(p / Q16_SCALE);                 /* LLR-FX-9 */
}

sc_q16_t sc_q16_div(sc_q16_t a, sc_q16_t b)
{
    sc_q16_t result;

    if (b == 0)
    {
        result = (a >= 0) ? SC_Q16_MAX : SC_Q16_MIN; /* LLR-FX-10 */
    }
    else
    {
        int64_t n = (int64_t)a * Q16_SCALE;
        result = clamp_i64(n / (int64_t)b);          /* LLR-FX-11 */
    }

    return result;
}

sc_q16_t sc_q16_abs(sc_q16_t q)
{
    sc_q16_t result;

    if (q == SC_Q16_MIN)
    {
        result = SC_Q16_MAX;                         /* LLR-FX-16 */
    }
    else if (q < 0)
    {
        result = -q;                                 /* LLR-FX-17 */
    }
    else
    {
        result = q;                                  /* LLR-FX-18 */
    }

    return result;
}

sc_q16_t sc_q16_clamp(sc_q16_t q, sc_q16_t lo, sc_q16_t hi)
{
    sc_q16_t result;

    if (lo > hi)
    {
        result = lo;                                 /* LLR-FX-12 */
    }
    else if (q < lo)
    {
        result = lo;                                 /* LLR-FX-19 */
    }
    else if (q > hi)
    {
        result = hi;                                 /* LLR-FX-20 */
    }
    else
    {
        result = q;                                  /* LLR-FX-21 */
    }

    return result;
}
