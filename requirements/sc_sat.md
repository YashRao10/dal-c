# sc_sat — Requirements

Saturating 32-bit integer add and subtract. Pure functions, no state.

Verified by: `tests/test_sc_sat.c`

## High-Level Requirements

- **HLR-SAT-1** — When the exact mathematical result of the operation is
  within `[INT32_MIN, INT32_MAX]`, the function shall return that result.
- **HLR-SAT-2** — When the exact result exceeds `INT32_MAX` the function
  shall return `INT32_MAX`; when it is below `INT32_MIN` it shall return
  `INT32_MIN`. The function shall never evaluate a signed expression that
  overflows.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-SAT-1 | HLR-SAT-2 | `sc_sat_add_i32` returns `INT32_MAX` when `b > 0` and `a > INT32_MAX - b`. | `sc_sat_add_i32` |
| LLR-SAT-2 | HLR-SAT-2 | `sc_sat_add_i32` returns `INT32_MIN` when `b < 0` and `a < INT32_MIN - b`. | `sc_sat_add_i32` |
| LLR-SAT-3 | HLR-SAT-1 | Otherwise `sc_sat_add_i32` returns `a + b`. | `sc_sat_add_i32` |
| LLR-SAT-4 | HLR-SAT-2 | `sc_sat_sub_i32` returns `INT32_MAX` when `b < 0` and `a > INT32_MAX + b`. | `sc_sat_sub_i32` |
| LLR-SAT-5 | HLR-SAT-2 | `sc_sat_sub_i32` returns `INT32_MIN` when `b > 0` and `a < INT32_MIN + b`. | `sc_sat_sub_i32` |
| LLR-SAT-6 | HLR-SAT-1 | Otherwise `sc_sat_sub_i32` returns `a - b`. | `sc_sat_sub_i32` |

## Design notes

Each overflow test is arranged so no sub-expression can itself overflow:
for addition, `INT32_MAX - b` is safe when `b > 0` and `INT32_MIN - b` is
safe when `b < 0`; subtraction is symmetric with `+ b`.
