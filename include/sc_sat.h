/*
 * sc_sat.h -- saturating 32-bit integer arithmetic.
 *
 * Add and subtract that pin to INT32_MIN / INT32_MAX on overflow instead of
 * wrapping (which is undefined behaviour for signed types in C). Pure
 * functions, no state, no branches on data beyond the overflow checks.
 *
 * These are split out from sc_ratelimit so the overflow behaviour can be
 * exercised directly for full MC/DC rather than only through a caller that
 * cannot reach the extremes.
 */
#ifndef DAL_C_SC_SAT_H
#define DAL_C_SC_SAT_H

#include "sc_common.h"

/* Returns a + b, clamped to [INT32_MIN, INT32_MAX]. Never invokes UB. */
int32_t sc_sat_add_i32(int32_t a, int32_t b);

/* Returns a - b, clamped to [INT32_MIN, INT32_MAX]. Never invokes UB. */
int32_t sc_sat_sub_i32(int32_t a, int32_t b);

#endif /* DAL_C_SC_SAT_H */
