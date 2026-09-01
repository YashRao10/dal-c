# PROMPT_INPUTS — dal-c

Raw, verbatim user prompts that shaped this project. Newest at the bottom.

---

**2026-08-31** (during a portfolio/career session, after reviewing the
Command Deck status survey):

> Lets look at the command deck and then see what needs to be added and new
> project can be created. I've seen our GitHub and it's good but it's a lot
> of python and html maybe we should move towards some c projects

> Do some research on the c projects we can do for all avenues and areas of
> our work

(Research produced a ranked survey across the safety-critical / DO-178C
lane, the AI-in-regulated-contexts lane, the markets/econ lane, and CS
fundamentals. Recommendation: a DAL-C-style reusable C component library.)

> ok yeah go ahead with dal c project is this going to be a library that we
> can see?

> yeah that one is fine go ahead and get started on that process

(Confirmed: name `dal-c`, plain Makefile, GitHub Pages face showing a live
coverage report + traceability matrix.)

> sorry just keep it here no backgrounding just working

---

**2026-09-01** (resuming after a session restart):

> welcome back had to restart you can continue working on Dal-c library and
> making the changes

(Asked which direction; user picked "New component". Added `sc_median` —
a sliding-window median filter for impulse-noise rejection — through the
full pipeline: requirements, LLR tags, requirements-based tests, 100% MC/DC,
fuzz invariant, RTM/verification regen, and the docs-site counts.)

---

**2026-09-01** (later, during a Robinhood/financials session that pivoted
back to dal-c):

> ok you can start doing more updates on dal c and more updates

> ok you can keep working

(Added `sc_vote` — an M-of-N redundancy voter: clusters replicated channel
readings by a tolerance, returns the majority value, the agreeing count, a
dissenting-channel bitmask, and a consensus verdict. Full pipeline:
5 HLR / 14 LLR, requirements-based suite with MC/DC cases for every compound
decision, a fuzz invariant, RTM/verification/coverage regen, README +
landing + architecture count updates. 14 components, 67 HLR / 170 LLR,
2426 checks, 100% line + 100% MC/DC held.)

---

**2026-09-01** (later still, after emailing the PACT MCP demo recording,
the local Pages site open in a browser):

> you can naviaget to dal c and keep working

> local and keep building before github

(Added `sc_watchdog` — a latching deadline supervisor that pairs with
`sc_sched`: a supervised activity must kick once per `timeout` ticks or the
watchdog trips and stays tripped until re-armed. Full pipeline: 6 HLR /
12 LLR, 9-test suite (53 checks) with MC/DC on both compound decisions, a
fuzz invariant (latch monotonic, elapsed pinned at the ceiling). Also
fixed `gen_rtm.py`'s component→test-file map, which had been showing
cosmetic GAP badges for `sc_debounce` / `sc_fixed` / `sc_sched`.
15 components, 73 HLR / 182 LLR, 2479 checks, 543 lines / 374 conditions,
100% line + 100% MC/DC held.)

---

## Scope decisions made during the build

- v1 components: `sc_hysteresis`, `sc_ringbuf`, `sc_ratelimit`. Grew to four
  when the saturating-arithmetic paths inside `sc_ratelimit` turned out to
  be unreachable through its public API — extracted them into `sc_sat` so
  the overflow behaviour could be tested directly for full MC/DC.
- `sc_hysteresis` generalises the Low Fuel Warning demo function
  (`PACT Work/05-Demo-Projects/LFW-DO178-Demo`) from a fixed-polarity
  low-fuel latch into a generic Schmitt trigger.
- Verification bar set at 100% line + 100% branch/condition (MC/DC),
  CI-enforced, plus a traceability generator that fails on any orphan.
