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
