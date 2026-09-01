# sc_watchdog — Requirements

Deadline supervisor for a periodic activity. The activity must "kick" the
watchdog at least once every `timeout` ticks; the application advances the
watchdog one tick at a time. If `timeout` ticks pass with no intervening
kick the watchdog trips, and the tripped state latches — a kick or a
subsequent in-deadline tick does not clear it. Only re-initialisation does.

One state struct per supervised activity. No dynamic memory; the elapsed
counter is bounded by `timeout` so its increment cannot overflow.

Verified by: `tests/test_sc_watchdog.c`

## High-Level Requirements

- **HLR-WDG-1** — `sc_watchdog_config_valid` shall return `SC_ERR_NULL` for
  a NULL config, `SC_ERR_PARAM` for a `timeout` of 0, and `SC_OK`
  otherwise.
- **HLR-WDG-2** — `sc_watchdog_init` shall set the elapsed count to 0 and
  the tripped flag to false, and shall return `SC_ERR_NULL` for a NULL
  state.
- **HLR-WDG-3** — `sc_watchdog_kick` shall set the elapsed count to 0 for a
  non-NULL state and do nothing for a NULL state. It shall not clear the
  tripped flag.
- **HLR-WDG-4** — `sc_watchdog_tick` shall return `SC_WATCHDOG_TRIPPED`
  without modifying the state when either `cfg` or `state` is NULL.
- **HLR-WDG-5** — When neither argument is NULL, `sc_watchdog_tick` shall
  increment the elapsed count by one while it is below `cfg->timeout` and
  leave it at `cfg->timeout` once reached, shall set the tripped flag when
  the elapsed count reaches `cfg->timeout`, and shall return
  `SC_WATCHDOG_TRIPPED` if the tripped flag is set or `SC_WATCHDOG_OK`
  otherwise. The increment shall not overflow.
- **HLR-WDG-6** — Once set, the tripped flag shall remain set until
  `sc_watchdog_init` is called; neither a kick nor an in-deadline tick
  shall clear it. `sc_watchdog_expired` shall return true exactly while the
  flag is set for a non-NULL state, and false for a NULL state.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-WDG-1 | HLR-WDG-1 | `sc_watchdog_config_valid` returns `SC_ERR_NULL` for a NULL `cfg`. | `sc_watchdog_config_valid` |
| LLR-WDG-2 | HLR-WDG-1 | Returns `SC_ERR_PARAM` when `cfg->timeout` is 0. | `sc_watchdog_config_valid` |
| LLR-WDG-3 | HLR-WDG-1 | Returns `SC_OK` when `cfg` is non-NULL and `timeout` is non-zero. | `sc_watchdog_config_valid` |
| LLR-WDG-4 | HLR-WDG-2 | `sc_watchdog_init` returns `SC_ERR_NULL` for a NULL `state`. | `sc_watchdog_init` |
| LLR-WDG-5 | HLR-WDG-2 | `sc_watchdog_init` sets `elapsed` to 0 and `tripped` to false. | `sc_watchdog_init` |
| LLR-WDG-6 | HLR-WDG-3 | `sc_watchdog_kick` returns without effect when `state` is NULL. | `sc_watchdog_kick` |
| LLR-WDG-7 | HLR-WDG-3 | `sc_watchdog_kick` sets `elapsed` to 0 for a non-NULL `state`, leaving `tripped` unchanged. | `sc_watchdog_kick` |
| LLR-WDG-8 | HLR-WDG-4 | `sc_watchdog_tick` returns `SC_WATCHDOG_TRIPPED` without modifying the state when `cfg` or `state` is NULL. | `sc_watchdog_tick` |
| LLR-WDG-9 | HLR-WDG-5 | When `elapsed < timeout`, `sc_watchdog_tick` increments `elapsed` by one (widened to `unsigned` for the sum so the `uint16_t` store cannot overflow); otherwise it leaves `elapsed` at `timeout`. | `sc_watchdog_tick` |
| LLR-WDG-10 | HLR-WDG-5, HLR-WDG-6 | After the increment, `sc_watchdog_tick` sets `tripped` when `elapsed >= timeout`; no path in `sc_watchdog_tick` or `sc_watchdog_kick` clears it. | `sc_watchdog_tick` |
| LLR-WDG-11 | HLR-WDG-5 | `sc_watchdog_tick` returns `SC_WATCHDOG_TRIPPED` when `tripped` is set, otherwise `SC_WATCHDOG_OK`. | `sc_watchdog_tick` |
| LLR-WDG-12 | HLR-WDG-6 | `sc_watchdog_expired` returns true only when `state` is non-NULL and `state->tripped` is set. | `sc_watchdog_expired` |

## Design notes

The elapsed counter counts *up* to a ceiling, unlike a hardware watchdog's
down-counter, but the contract is the same: `timeout` is the largest number
of ticks the supervised activity may go without checking in. `timeout == 1`
is permitted but degenerate — the activity must kick before every tick.

The tripped flag is written `true` in exactly one statement and `false` in
exactly one other (`sc_watchdog_init`). That is the entire latch; there is
no clear-on-recovery path to reason about.

Pairs with `sc_sched`: a slot that supervises another can call
`sc_watchdog_tick` each cycle and raise an alarm on `SC_WATCHDOG_TRIPPED`.
