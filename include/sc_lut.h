/*
 * sc_lut.h -- piecewise-linear lookup table with clamped extrapolation.
 *
 * Maps an input through a table of (x, y) breakpoints, interpolating
 * linearly between them and clamping to the end values outside the table
 * range. Used to linearise a sensor curve, shape a command, or apply a
 * calibration.
 *
 * The breakpoint arrays are owned by the caller and must outlive the
 * table. `x` must be strictly increasing. Interpolation uses one int64_t
 * intermediate so a wide y-range cannot overflow.
 */
#ifndef DAL_C_SC_LUT_H
#define DAL_C_SC_LUT_H

#include "sc_common.h"

typedef struct
{
    const int32_t *x;   /* strictly increasing breakpoints */
    const int32_t *y;   /* value at each breakpoint         */
    uint32_t       n;   /* number of points, >= 2           */
} sc_lut_t;

/*
 * Validate a table.
 *   SC_OK        usable
 *   SC_ERR_NULL  lut, x or y is NULL
 *   SC_ERR_PARAM n < 2, or x is not strictly increasing
 */
SC_NODISCARD sc_status_t sc_lut_valid(const sc_lut_t *lut);

/*
 * Evaluate the table at `x`. Below x[0] returns y[0]; at or above x[n-1]
 * returns y[n-1]; between, the linear interpolation of the bracketing
 * segment. Returns 0 if `lut` is NULL. Assumes the table has passed
 * sc_lut_valid().
 */
int32_t sc_lut_eval(const sc_lut_t *lut, int32_t x);

#endif /* DAL_C_SC_LUT_H */
