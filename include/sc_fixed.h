/*
 * sc_fixed.h -- signed Q16.16 fixed-point arithmetic.
 *
 * A value is an int32_t interpreted as (integer_bits << 16) | fraction.
 * All operations saturate at the representable limits rather than wrap.
 * Multiply and divide use a single int64_t intermediate; everything else
 * is 32-bit.
 */
#ifndef DAL_C_SC_FIXED_H
#define DAL_C_SC_FIXED_H

#include "sc_common.h"

typedef int32_t sc_q16_t;

#define SC_Q16_SHIFT  16
#define SC_Q16_ONE    ((sc_q16_t)0x00010000)
#define SC_Q16_MAX    ((sc_q16_t)0x7FFFFFFF)   /* ~ +32767.99998 */
#define SC_Q16_MIN    ((sc_q16_t)(-2147483647 - 1))  /* -32768.0 */

/* Whole number <-> Q16. from_int saturates values outside +/-32767. */
sc_q16_t sc_q16_from_int(int32_t value);

/* Truncates toward zero. */
int32_t  sc_q16_to_int(sc_q16_t q);

/* Saturating add / subtract. */
sc_q16_t sc_q16_add(sc_q16_t a, sc_q16_t b);
sc_q16_t sc_q16_sub(sc_q16_t a, sc_q16_t b);

/* Saturating multiply, round-to-nearest (ties away from zero). */
sc_q16_t sc_q16_mul(sc_q16_t a, sc_q16_t b);

/* Saturating divide. b == 0 yields SC_Q16_MAX or SC_Q16_MIN by the sign of
 * a (and SC_Q16_MAX when a == 0). */
sc_q16_t sc_q16_div(sc_q16_t a, sc_q16_t b);

/* Absolute value, saturating (|SC_Q16_MIN| -> SC_Q16_MAX). */
sc_q16_t sc_q16_abs(sc_q16_t q);

/* Clamp q to [lo, hi]. If lo > hi the result is lo. */
sc_q16_t sc_q16_clamp(sc_q16_t q, sc_q16_t lo, sc_q16_t hi);

#endif /* DAL_C_SC_FIXED_H */
