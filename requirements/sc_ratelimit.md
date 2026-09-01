# sc_ratelimit — Requirements

A slew-rate limiter with output clamp. Moves an internal output toward a
commanded target by at most a fixed step per update, then clamps it.

Verified by: `tests/test_sc_ratelimit.c` (with `sc_sat` providing the
saturating arithmetic — see `requirements/sc_sat.md`).

## High-Level Requirements

- **HLR-RL-1** — A commanded change larger than the configured limit shall
  be applied as at most `max_step_up` per update when rising, or
  `max_step_down` per update when falling.
- **HLR-RL-2** — A commanded change within the configured limits shall be
  applied in full on that update.
- **HLR-RL-3** — Given a fixed reachable target, the output shall converge
  onto it exactly within a finite number of updates and then hold.
- **HLR-RL-4** — The returned output shall always lie in
  `[out_min, out_max]`, regardless of the starting value or target.
- **HLR-RL-5** — No input value shall cause the output to wrap; internal
  arithmetic shall saturate at the 32-bit limits.
- **HLR-RL-6** — If `state` is NULL the function shall return 0; if `state`
  is non-NULL but `cfg` is NULL it shall return the current output unchanged.
- **HLR-RL-7** — `sc_ratelimit_config_valid` shall reject a negative step
  or `out_min > out_max`.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-RL-1  | HLR-RL-1, HLR-RL-5 | `delta` is `sc_sat_sub_i32(target, state->output)`. | `sc_ratelimit_update` |
| LLR-RL-2  | HLR-RL-1 | `delta` is limited to `max_step_up` when it exceeds it. | `sc_ratelimit_update` |
| LLR-RL-3  | HLR-RL-1 | `delta` is limited to `-max_step_down` when it is below it. | `sc_ratelimit_update` |
| LLR-RL-4  | HLR-RL-2 | `delta` within `[-max_step_down, max_step_up]` is left unchanged. | `sc_ratelimit_update` |
| LLR-RL-5  | HLR-RL-5 | The new output is `sc_sat_add_i32(state->output, delta)`. | `sc_ratelimit_update` |
| LLR-RL-6  | HLR-RL-6 | NULL `state` returns 0; NULL `cfg` with non-NULL `state` returns `state->output`. | `sc_ratelimit_update` |
| LLR-RL-7  | HLR-RL-4 | The new output is raised to `out_min` if below it. | `sc_ratelimit_update` |
| LLR-RL-8  | HLR-RL-4 | The new output is lowered to `out_max` if above it. | `sc_ratelimit_update` |
| LLR-RL-9  | HLR-RL-3 | `state->output` is updated to the clamped value and returned. | `sc_ratelimit_update` |
| LLR-RL-11 | HLR-RL-7 | `sc_ratelimit_config_valid` returns `SC_ERR_NULL` for NULL `cfg`. | `sc_ratelimit_config_valid` |
| LLR-RL-12 | HLR-RL-7 | Returns `SC_ERR_PARAM` when `max_step_up < 0` or `max_step_down < 0`. | `sc_ratelimit_config_valid` |
| LLR-RL-13 | HLR-RL-7 | Returns `SC_ERR_PARAM` when `out_min > out_max`. | `sc_ratelimit_config_valid` |
| LLR-RL-14 | HLR-RL-7 | Returns `SC_OK` otherwise. | `sc_ratelimit_config_valid` |
| LLR-RL-15 | HLR-RL-6 | `sc_ratelimit_init` returns `SC_ERR_NULL` for NULL `state`. | `sc_ratelimit_init` |
| LLR-RL-16 | HLR-RL-2 | `sc_ratelimit_init` sets `state->output` to `initial` and returns `SC_OK`. | `sc_ratelimit_init` |
| LLR-RL-17 | HLR-RL-4 | A new output already within `[out_min, out_max]` is left unchanged. | `sc_ratelimit_update` |
| LLR-RL-18 | HLR-RL-6 | `sc_ratelimit_output` returns 0 for NULL `state`. | `sc_ratelimit_output` |
| LLR-RL-19 | HLR-RL-3 | `sc_ratelimit_output` returns `state->output` otherwise. | `sc_ratelimit_output` |

## Design notes

`delta` after clamping is always in `[-max_step_down, max_step_up]`, both
within `int32_t`, so `sc_sat_add_i32(output, delta)` cannot itself be
reached at its saturation points through this caller — which is exactly why
`sc_sat` is a separate component with its own direct tests (see
`requirements/sc_sat.md`). Precondition: `cfg` has passed
`sc_ratelimit_config_valid`, so `max_step_down` is non-negative and
`-max_step_down` does not overflow.

(LLR-RL-10 is intentionally unused — the numbering was left stable when the
saturating helpers were extracted into `sc_sat`.)
