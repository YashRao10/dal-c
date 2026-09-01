# sc_pid — Requirements

Positional PID controller in Q16.16 with derivative-on-measurement,
derivative low-pass, output clamping, and conditional-integration
anti-windup.

Verified by: `tests/test_sc_pid.c` (fixed-point arithmetic from `sc_fixed`
/ `sc_sat` — see those requirements).

## High-Level Requirements

- **HLR-PID-1** — `sc_pid_config_valid` shall reject a NULL config,
  `out_min > out_max`, or a derivative-filter coefficient outside
  `[0, SC_Q16_ONE)`.
- **HLR-PID-2** — `sc_pid_init` shall preload the integrator with the given
  initial output and seed the measurement history; a NULL state shall be
  rejected. `sc_pid_update` shall return 0 for a NULL config or state and
  not modify the state.
- **HLR-PID-3** — The proportional term shall be `kp × (setpoint −
  measurement)`.
- **HLR-PID-4** — The derivative term shall be computed from the change in
  *measurement*, not error, so a setpoint step alone produces no
  derivative contribution.
- **HLR-PID-5** — The derivative term shall be passed through a first-order
  low-pass with coefficient `d_filter`.
- **HLR-PID-6** — The integrator shall accumulate `ki × error` each cycle,
  subject to anti-windup.
- **HLR-PID-7** — While the unclamped output is above `out_max` (or below
  `out_min`), the integrator shall be updated only if the update moves the
  output toward the band; otherwise it shall be held.
- **HLR-PID-8** — The returned output shall always lie within
  `[out_min, out_max]`, and a stable closed loop shall converge onto its
  setpoint.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-PID-1  | HLR-PID-2 | `sc_pid_update` returns 0 without touching the state when `cfg` or `state` is NULL. | `sc_pid_update` |
| LLR-PID-2  | HLR-PID-3 | `error = setpoint − measurement` (saturating). | `sc_pid_update` |
| LLR-PID-3  | HLR-PID-3 | `p_term = kp × error`. | `sc_pid_update` |
| LLR-PID-4  | HLR-PID-4 | `d_raw = kd × −(measurement − prev_measurement)`. | `sc_pid_update` |
| LLR-PID-5  | HLR-PID-5 | `d_term = d_filter × prev_derivative + (1 − d_filter) × d_raw`. | `sc_pid_update` |
| LLR-PID-6  | HLR-PID-6 | `i_new = integrator + ki × error`. | `sc_pid_update` |
| LLR-PID-7  | HLR-PID-8 | `out_raw = p_term + i_new + d_term`. | `sc_pid_update` |
| LLR-PID-8  | HLR-PID-7, HLR-PID-8 | When `out_raw > out_max`: return `out_max`; commit `i_new` only if `i_new <= integrator`. | `sc_pid_update` |
| LLR-PID-9  | HLR-PID-7, HLR-PID-8 | When `out_raw < out_min`: return `out_min`; commit `i_new` only if `i_new >= integrator`. | `sc_pid_update` |
| LLR-PID-10 | HLR-PID-8 | Otherwise return `out_raw` and commit `i_new`. | `sc_pid_update` |
| LLR-PID-11 | HLR-PID-1 | `sc_pid_config_valid` returns `SC_ERR_NULL` for NULL `cfg`. | `sc_pid_config_valid` |
| LLR-PID-12 | HLR-PID-1 | Returns `SC_ERR_PARAM` when `out_min > out_max`. | `sc_pid_config_valid` |
| LLR-PID-13 | HLR-PID-1 | Returns `SC_ERR_PARAM` when `d_filter < 0` or `d_filter >= SC_Q16_ONE`. | `sc_pid_config_valid` |
| LLR-PID-14 | HLR-PID-1 | Returns `SC_OK` otherwise. | `sc_pid_config_valid` |
| LLR-PID-15 | HLR-PID-2 | `sc_pid_init` returns `SC_ERR_NULL` for NULL `state`. | `sc_pid_init` |
| LLR-PID-16 | HLR-PID-2 | `sc_pid_init` sets `integrator = initial_output`, `prev_measurement = initial_measurement`, `prev_derivative = 0`. | `sc_pid_init` |

## Design notes

The sample period is folded into the gains (`ki` = Ki·dt, `kd` = Kd/dt) so
the update contains no `dt` arithmetic. Anti-windup is conditional
integration: the integrator is frozen, not back-calculated, while the
output is railed and the error would drive it further in. Derivative on
measurement removes derivative kick on a setpoint change.
