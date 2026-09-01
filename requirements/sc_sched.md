# sc_sched — Requirements

Cooperative cyclic scheduler.

Verified by: `tests/test_sc_sched.c`

## High-Level Requirements

- **HLR-SCH-1** — `sc_sched_config_valid` shall reject a NULL config, a
  NULL slot table, zero slots, a NULL task pointer, a period of 0, or a
  phase not less than its period.
- **HLR-SCH-2** — `sc_sched_init` shall set the tick counter to 0; a NULL
  state shall be rejected. `sc_sched_tick` shall return 0 and not advance
  the tick for a NULL config or state.
- **HLR-SCH-3** — On each `sc_sched_tick`, every slot for which
  `tick % period == phase` shall be run once, in table order, and the
  tick counter shall then advance by one. The return value shall be the
  number of tasks run.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-SCH-1 | HLR-SCH-2 | `sc_sched_init` sets `state->tick` to 0. | `sc_sched_init` |
| LLR-SCH-2 | HLR-SCH-3 | `sc_sched_tick` iterates the slots `0 .. n-1` in order. | `sc_sched_tick` |
| LLR-SCH-3 | HLR-SCH-3 | A slot is run when `(tick % period) == phase`. | `sc_sched_tick` |
| LLR-SCH-4 | HLR-SCH-2 | A NULL `cfg` or `state` returns 0 without running or advancing. | `sc_sched_tick` |
| LLR-SCH-5 | HLR-SCH-2 | `sc_sched_init` returns `SC_ERR_NULL` for a NULL state. | `sc_sched_init` |
| LLR-SCH-6 | HLR-SCH-1 | `sc_sched_config_valid` returns `SC_ERR_NULL` for a NULL `cfg`, a NULL `slots`, or a NULL slot `run`. | `sc_sched_config_valid` |
| LLR-SCH-7 | HLR-SCH-1 | Returns `SC_ERR_PARAM` when `n == 0`. | `sc_sched_config_valid` |
| LLR-SCH-8 | HLR-SCH-1 | Returns `SC_ERR_PARAM` when a slot has `period == 0` or `phase >= period`. | `sc_sched_config_valid` |
| LLR-SCH-9 | HLR-SCH-3 | After running the due slots, `state->tick` is incremented and the run count returned. | `sc_sched_tick` |

## Design notes

The scheduler runs tasks to completion in table order with no preemption,
so the caller reasons about timing directly. There is no drift: the
schedule is purely a function of the tick count. Table order doubles as
priority when two tasks are due on the same tick.
