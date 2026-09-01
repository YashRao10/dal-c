# dal-c

A small library of reusable C components built to a **Design Assurance Level C**
style: requirements-driven, MC/DC-covered, statically analysed, no dynamic
memory, no recursion, no unbounded loops.

[![ci](https://github.com/YashRao10/dal-c/actions/workflows/ci.yml/badge.svg)](https://github.com/YashRao10/dal-c/actions/workflows/ci.yml)
![coverage](https://img.shields.io/badge/coverage-100%25%20line%20%2F%20100%25%20MC%2FDC-2f8f5b)
![C99](https://img.shields.io/badge/C-C99%20freestanding-6b8eaa)
![license](https://img.shields.io/badge/license-MIT-8b949e)

> Not affiliated with any airframer or certification authority. This is a
> demonstration of the DO-178C verification *process* applied to real code —
> not certified software.

## Components

| Module | Purpose |
|---|---|
| `sc_sat` | Saturating 32-bit add / subtract — pins at `INT32_MIN` / `INT32_MAX` instead of invoking signed-overflow UB. |
| `sc_hysteresis` | Schmitt-trigger comparator with independent assert / clear thresholds. A noisy or slowly-drifting signal near the trip point cannot chatter the output. |
| `sc_ringbuf` | Fixed-capacity byte FIFO over caller-provided storage. No `malloc`, no power-of-two constraint, unambiguous full / empty. |
| `sc_ratelimit` | Slew-rate limiter with output clamp. Bounds how fast a command signal can change and keeps it within range, on saturating arithmetic. |

Each module is one `.h` in `include/` and one `.c` in `src/`, depends only on
`<stdint.h>` / `<stdbool.h>` / `<stddef.h>`, and owns no static state — the
caller passes an instance struct by pointer on every call.

## Build and test

```sh
make            # build + run the test suite (strict warnings, -Werror)
make coverage   # rebuild instrumented, run tests, print statement/branch/MC-DC gcov
make clean
```

Requires a C99 compiler; `make coverage` needs GCC ≥ 14 for condition
(MC/DC) coverage. No other dependencies.

## How each component is built

1. **Requirements** — `requirements/<module>.md` states high-level
   requirements, then low-level requirements that each trace up to one or
   more HLRs and name the implementing function.
2. **Code** — every implementation block carries an `/* LLR-... */` tag, so
   the trace between requirement and source is bidirectional.
3. **Requirements-based tests** — `tests/test_<module>.c`; each test names
   the requirement it verifies. Compound decisions get cases chosen so each
   condition is shown to independently affect the outcome (MC/DC).
4. **Structural coverage** — CI runs `make coverage` and fails below 100%
   line and 100% branch / condition coverage on `src/`.
5. **Traceability** — `tools/gen_rtm.py` regenerates
   [`docs/traceability.html`](docs/traceability.html) and fails the build on
   any orphan requirement, unimplemented LLR, or untested HLR.
6. **Static analysis** — `cppcheck` on every push.

The generated coverage report and traceability matrix are published to
GitHub Pages by CI.

## Coding style

DAL-C-flavoured, not a certified process:

- No dynamic memory, no recursion, no unbounded loops.
- Single point of exit per function where practical.
- All `if … else if` chains terminate in `else` (MISRA C:2012 Rule 15.7).
- Fixed-width integer types; explicit `== true` / `== false` on the two
  compound decisions so their MC/DC obligations are unambiguous.
- MISRA C:2012 is the target standard. The one deliberate deviation
  (`__attribute__((warn_unused_result))`, Rule 1.2) is documented at its
  definition in `include/sc_common.h`.

## Layout

```
include/     public headers (one per component + sc_common.h)
src/         implementations, each block tagged with its LLR
requirements/ per-component HLR/LLR specs
tests/       dependency-free harness + one requirements-based suite per component
tools/       gen_rtm.py — traceability matrix generator
docs/        GitHub Pages site (landing + generated RTM + coverage)
```

## License

MIT — see [LICENSE](LICENSE).
