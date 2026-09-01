# sc_sm — Requirements

Table-driven finite state machine engine.

Verified by: `tests/test_sc_sm.c`

## High-Level Requirements

- **HLR-SM-1** — `sc_sm_config_valid` shall reject a NULL config, a NULL
  transition table, or a table of zero rows.
- **HLR-SM-2** — `sc_sm_init` shall set the machine to `cfg->initial`;
  `sc_sm_state` shall report the current state id, or `0xFFFF` for a NULL
  state. NULL arguments to either shall be rejected / handled.
- **HLR-SM-3** — On an event, `sc_sm_dispatch` shall use the first table
  row whose `from` equals the current state and whose `event` equals the
  delivered event: it shall run that row's action (if non-NULL), move to
  the row's `to` state, and return true.
- **HLR-SM-4** — If no row matches, the state shall not change, no action
  shall run, and `sc_sm_dispatch` shall return false. NULL arguments shall
  return false.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-SM-1  | HLR-SM-2 | `sc_sm_init` sets `state->state` to `cfg->initial`. | `sc_sm_init` |
| LLR-SM-2  | HLR-SM-3, HLR-SM-4 | `sc_sm_dispatch` scans rows `0 .. n-1` in order. | `sc_sm_dispatch` |
| LLR-SM-3  | HLR-SM-3 | On a match with a non-NULL action, the action is called with `ctx`. | `sc_sm_dispatch` |
| LLR-SM-4  | HLR-SM-3 | On a match, `state->state` becomes `t->to`, `fired` is set, and the scan stops. | `sc_sm_dispatch` |
| LLR-SM-5  | HLR-SM-4 | A NULL `cfg` or `state` returns false without scanning. | `sc_sm_dispatch` |
| LLR-SM-6  | HLR-SM-2 | `sc_sm_init` returns `SC_ERR_NULL` when `state` or `cfg` is NULL. | `sc_sm_init` |
| LLR-SM-7  | HLR-SM-1 | `sc_sm_config_valid` returns `SC_ERR_NULL` when `cfg` or `cfg->table` is NULL. | `sc_sm_config_valid` |
| LLR-SM-8  | HLR-SM-1 | Returns `SC_ERR_PARAM` when `n == 0`. | `sc_sm_config_valid` |
| LLR-SM-9  | HLR-SM-1 | Returns `SC_OK` otherwise. | `sc_sm_config_valid` |
| LLR-SM-10 | HLR-SM-2 | `sc_sm_state` returns `0xFFFF` for a NULL state. | `sc_sm_state` |
| LLR-SM-11 | HLR-SM-2 | `sc_sm_state` returns `state->state` otherwise. | `sc_sm_state` |

## Design notes

The engine owns only a `uint16_t` state id; the transition table and any
context are the caller's. The scan is a single bounded linear pass, so
dispatch is `O(n)` with a deterministic worst case. First-match-wins lets
the caller order rows by priority. There are no entry/exit actions or
guards in this version — a transition action plus the destination state
covers the common cases.
