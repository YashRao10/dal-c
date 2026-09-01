# sc_hysteresis — Requirements

A single-input Boolean comparator with independent assert and clear
thresholds (a Schmitt trigger).

Verified by: `tests/test_sc_hysteresis.c`

## High-Level Requirements

- **HLR-HYS-1** — After initialisation the output shall be CLEARED (false).
- **HLR-HYS-2** — The output shall ASSERT (true) when the input rises to or
  above `high_threshold` while the output is currently cleared.
- **HLR-HYS-3** — The output shall CLEAR when the input falls to or below
  `low_threshold` while the output is currently asserted.
- **HLR-HYS-4** — While the input is strictly between the two thresholds the
  output shall hold its previous value.
- **HLR-HYS-5** — Across any sequence of inputs the output shall change at
  most once per update and only in the direction the input crossing implies.
- **HLR-HYS-6** — A configuration with `high_threshold == low_threshold`
  shall behave as a plain comparator (no dead band).
- **HLR-HYS-7** — A configuration with `high_threshold < low_threshold`
  shall be reported invalid by `sc_hysteresis_config_valid`.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-HYS-1  | HLR-HYS-1 | `sc_hysteresis_init` sets `state->asserted` to `false`. | `sc_hysteresis_init` |
| LLR-HYS-2  | HLR-HYS-2, HLR-HYS-5 | `sc_hysteresis_update` sets `asserted` true when `asserted` is false and `input >= cfg->high_threshold`. | `sc_hysteresis_update` |
| LLR-HYS-3  | HLR-HYS-3, HLR-HYS-5 | `sc_hysteresis_update` sets `asserted` false when `asserted` is true and `input <= cfg->low_threshold`. | `sc_hysteresis_update` |
| LLR-HYS-4  | HLR-HYS-4, HLR-HYS-5 | When neither edge condition holds, `sc_hysteresis_update` leaves `asserted` unchanged. | `sc_hysteresis_update` |
| LLR-HYS-5  | HLR-HYS-2, HLR-HYS-3 | `sc_hysteresis_update` returns `false` without dereferencing either pointer if `cfg` or `state` is NULL. | `sc_hysteresis_update` |
| LLR-HYS-6  | HLR-HYS-2, HLR-HYS-3 | `sc_hysteresis_update` returns the post-update value of `state->asserted`. | `sc_hysteresis_update` |
| LLR-HYS-7  | HLR-HYS-7 | `sc_hysteresis_config_valid` returns `SC_ERR_NULL` when `cfg` is NULL. | `sc_hysteresis_config_valid` |
| LLR-HYS-8  | HLR-HYS-7 | `sc_hysteresis_config_valid` returns `SC_ERR_PARAM` when `cfg->high_threshold < cfg->low_threshold`. | `sc_hysteresis_config_valid` |
| LLR-HYS-9  | HLR-HYS-6, HLR-HYS-7 | Otherwise `sc_hysteresis_config_valid` returns `SC_OK`. | `sc_hysteresis_config_valid` |
| LLR-HYS-10 | HLR-HYS-1 | `sc_hysteresis_init` returns `SC_ERR_NULL` when `state` is NULL, else `SC_OK`. | `sc_hysteresis_init` |
| LLR-HYS-11 | HLR-HYS-1 | `sc_hysteresis_output` returns `false` when `state` is NULL. | `sc_hysteresis_output` |
| LLR-HYS-12 | HLR-HYS-1 | `sc_hysteresis_output` returns `state->asserted` otherwise. | `sc_hysteresis_output` |

## Design notes

The two edge decisions in `sc_hysteresis_update` are compound
(`&&` of two conditions). The test cases in `test_sc_hysteresis.c` are
selected so each condition is shown to independently affect the outcome
(MC/DC).
