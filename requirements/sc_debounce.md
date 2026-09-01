# sc_debounce — Requirements

Integrator debounce for a noisy digital input.

Verified by: `tests/test_sc_debounce.c`

## High-Level Requirements

- **HLR-DB-1** — `sc_debounce_config_valid` shall reject a NULL config or
  a threshold of 0.
- **HLR-DB-2** — `sc_debounce_init` shall set the output to a given value
  and clear the run counter; a NULL state shall be rejected.
  `sc_debounce_update` shall return `false` for a NULL config or state
  without modifying the state.
- **HLR-DB-3** — The output shall follow the raw input only after the raw
  input has disagreed with the current output for `threshold` consecutive
  updates.
- **HLR-DB-4** — A disagreement run shorter than `threshold` shall not
  change the output, and the run counter shall reset on the first
  agreeing sample.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-DB-1 | HLR-DB-2 | `sc_debounce_init` sets `output` to 0/1 from `initial` and `counter` to 0. | `sc_debounce_init` |
| LLR-DB-2 | HLR-DB-4 | When the raw sample equals `output`, `counter` is set to 0. | `sc_debounce_update` |
| LLR-DB-3 | HLR-DB-3 | When the raw sample differs, `counter` is incremented. | `sc_debounce_update` |
| LLR-DB-4 | HLR-DB-3 | When `counter` reaches `threshold`, `output` takes the raw value and `counter` is reset. | `sc_debounce_update` |
| LLR-DB-5 | HLR-DB-2 | A NULL `cfg` or `state` returns `false` without touching the state. | `sc_debounce_update` |
| LLR-DB-6 | HLR-DB-1 | `sc_debounce_config_valid` returns `SC_ERR_NULL` for NULL `cfg`. | `sc_debounce_config_valid` |
| LLR-DB-7 | HLR-DB-1 | Returns `SC_ERR_PARAM` when `threshold == 0`. | `sc_debounce_config_valid` |
| LLR-DB-8 | HLR-DB-1 | Returns `SC_OK` otherwise. | `sc_debounce_config_valid` |
| LLR-DB-9 | HLR-DB-2 | `sc_debounce_init` returns `SC_ERR_NULL` for NULL `state`. | `sc_debounce_init` |
