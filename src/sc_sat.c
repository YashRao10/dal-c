/*
 * sc_sat.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_sat.md.
 *
 * Both checks rearrange the overflow test so no intermediate can itself
 * overflow: for `a + b`, INT32_MAX - b is safe when b > 0 and INT32_MIN - b
 * is safe when b < 0 (and symmetrically for subtraction).
 */
#include "sc_sat.h"

int32_t sc_sat_add_i32(int32_t a, int32_t b)
{
    int32_t result;

    if ((b > 0) && (a > (INT32_MAX - b)))
    {
        result = INT32_MAX;                  /* LLR-SAT-1 */
    }
    else if ((b < 0) && (a < (INT32_MIN - b)))
    {
        result = INT32_MIN;                  /* LLR-SAT-2 */
    }
    else
    {
        result = a + b;                      /* LLR-SAT-3 */
    }

    return result;
}

int32_t sc_sat_sub_i32(int32_t a, int32_t b)
{
    int32_t result;

    if ((b < 0) && (a > (INT32_MAX + b)))
    {
        result = INT32_MAX;                  /* LLR-SAT-4 */
    }
    else if ((b > 0) && (a < (INT32_MIN + b)))
    {
        result = INT32_MIN;                  /* LLR-SAT-5 */
    }
    else
    {
        result = a - b;                      /* LLR-SAT-6 */
    }

    return result;
}
