# sc_fixed — Requirements

Signed Q16.16 fixed-point arithmetic. All operations saturate.

Verified by: `tests/test_sc_fixed.c`

## High-Level Requirements

- **HLR-FX-1** — Conversion between whole numbers and Q16.16 shall be
  exact within the representable range and saturate outside it;
  Q16.16 → int shall truncate toward zero.
- **HLR-FX-2** — Add and subtract shall return the exact result when it
  fits and saturate at `SC_Q16_MAX` / `SC_Q16_MIN` otherwise.
- **HLR-FX-3** — Multiply shall round to nearest (ties away from zero) and
  saturate on overflow.
- **HLR-FX-4** — Divide shall saturate on overflow; division by zero shall
  return `SC_Q16_MAX` when the dividend is ≥ 0 and `SC_Q16_MIN` otherwise.
- **HLR-FX-5** — Absolute value shall saturate: `|SC_Q16_MIN|` returns
  `SC_Q16_MAX`.
- **HLR-FX-6** — Clamp shall constrain a value to `[lo, hi]`, returning
  `lo` when `lo > hi`.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-FX-1  | HLR-FX-1 | `sc_q16_from_int` returns `SC_Q16_MAX` when `value > 32767`. | `sc_q16_from_int` |
| LLR-FX-2  | HLR-FX-1 | Returns `SC_Q16_MIN` when `value < -32768`. | `sc_q16_from_int` |
| LLR-FX-3  | HLR-FX-1 | Otherwise returns `value * SC_Q16_ONE`. | `sc_q16_from_int` |
| LLR-FX-4  | HLR-FX-1 | `sc_q16_to_int` returns `q / SC_Q16_ONE` (truncates toward zero). | `sc_q16_to_int` |
| LLR-FX-5  | HLR-FX-2 | `sc_q16_add` is `sc_sat_add_i32`. | `sc_q16_add` |
| LLR-FX-6  | HLR-FX-2 | `sc_q16_sub` is `sc_sat_sub_i32`. | `sc_q16_sub` |
| LLR-FX-7  | HLR-FX-3 | Multiply adds a half-LSB bias before the shift when the 64-bit product is ≥ 0. | `sc_q16_mul` |
| LLR-FX-8  | HLR-FX-3 | Multiply subtracts a half-LSB bias when the product is < 0 (ties away from zero). | `sc_q16_mul` |
| LLR-FX-9  | HLR-FX-3 | Multiply divides the biased product by 2^16 and clamps the result. | `sc_q16_mul` |
| LLR-FX-10 | HLR-FX-4 | `sc_q16_div` returns `SC_Q16_MAX` (dividend ≥ 0) or `SC_Q16_MIN` (dividend < 0) when the divisor is 0. | `sc_q16_div` |
| LLR-FX-11 | HLR-FX-4 | Otherwise divide computes `(a << 16) / b` in 64-bit and clamps. | `sc_q16_div` |
| LLR-FX-12 | HLR-FX-6 | `sc_q16_clamp` returns `lo` when `lo > hi`. | `sc_q16_clamp` |
| LLR-FX-13 | HLR-FX-3, HLR-FX-4 | `clamp_i64` returns `SC_Q16_MAX` when the 64-bit value exceeds it. | `clamp_i64` |
| LLR-FX-14 | HLR-FX-3, HLR-FX-4 | `clamp_i64` returns `SC_Q16_MIN` when the value is below it. | `clamp_i64` |
| LLR-FX-15 | HLR-FX-3, HLR-FX-4 | `clamp_i64` returns the value cast to `sc_q16_t` otherwise. | `clamp_i64` |
| LLR-FX-16 | HLR-FX-5 | `sc_q16_abs` returns `SC_Q16_MAX` when `q == SC_Q16_MIN`. | `sc_q16_abs` |
| LLR-FX-17 | HLR-FX-5 | Returns `-q` when `q < 0`. | `sc_q16_abs` |
| LLR-FX-18 | HLR-FX-5 | Returns `q` when `q >= 0`. | `sc_q16_abs` |
| LLR-FX-19 | HLR-FX-6 | `sc_q16_clamp` returns `lo` when `q < lo`. | `sc_q16_clamp` |
| LLR-FX-20 | HLR-FX-6 | Returns `hi` when `q > hi`. | `sc_q16_clamp` |
| LLR-FX-21 | HLR-FX-6 | Returns `q` when it is already within `[lo, hi]`. | `sc_q16_clamp` |

## Design notes

Multiply and divide use one `int64_t` intermediate; everything else is
32-bit. `SC_Q16_MIN` is written as `(-2147483647 - 1)` so the literal is
never the (out-of-`int`-range) token `2147483648`.
