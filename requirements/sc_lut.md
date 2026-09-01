# sc_lut — Requirements

Piecewise-linear lookup table with clamped extrapolation.

Verified by: `tests/test_sc_lut.c` (`sc_sat` provides the final add).

## High-Level Requirements

- **HLR-LUT-1** — An input at or below the first breakpoint shall return
  the first table value; an input at or above the last breakpoint shall
  return the last value; a NULL table shall evaluate to 0.
- **HLR-LUT-2** — An input strictly between two breakpoints shall return
  the linear interpolation of the bracketing segment.
- **HLR-LUT-3** — `sc_lut_valid` shall reject a NULL table or array,
  fewer than two points, or breakpoints that are not strictly increasing.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-LUT-1  | HLR-LUT-1 | `sc_lut_eval` returns `y[0]` when `x <= x[0]`. | `sc_lut_eval` |
| LLR-LUT-2  | HLR-LUT-1 | Returns `y[n-1]` when the segment search reaches the last point. | `sc_lut_eval` |
| LLR-LUT-3  | HLR-LUT-2 | The search advances until `x < x[i+1]`, selecting segment `i`. | `sc_lut_eval` |
| LLR-LUT-4  | HLR-LUT-2 | `step = y[i+1] - y[i]` and `span = x[i+1] - x[i]` are computed in 64-bit. | `sc_lut_eval` |
| LLR-LUT-5  | HLR-LUT-2 | `offset = step * (x - x[i]) / span`. | `sc_lut_eval` |
| LLR-LUT-6  | HLR-LUT-2 | The result is `sc_sat_add_i32(y[i], offset)`. | `sc_lut_eval` |
| LLR-LUT-7  | HLR-LUT-1 | A NULL `lut` returns 0. | `sc_lut_eval` |
| LLR-LUT-8  | HLR-LUT-3 | `sc_lut_valid` returns `SC_ERR_NULL` when `lut`, `x`, or `y` is NULL. | `sc_lut_valid` |
| LLR-LUT-9  | HLR-LUT-3 | Returns `SC_ERR_PARAM` when `n < 2`. | `sc_lut_valid` |
| LLR-LUT-10 | HLR-LUT-3 | Returns `SC_ERR_PARAM` when any `x[i+1] <= x[i]`. | `sc_lut_valid` |

## Design notes

The segment search is a bounded linear scan (`O(n)`, deterministic). The
interpolation numerator uses one `int64_t` intermediate so a wide y-range
cannot overflow before the divide brings it back into `int32_t`.
