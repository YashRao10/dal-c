# dal-c

A library of reusable C99 components built to a **Design Assurance Level C**
style: requirements-driven, 100% MC/DC-covered, statically analysed, no
dynamic memory, no recursion, no unbounded loops, saturating arithmetic
throughout.

[![ci](https://github.com/YashRao10/dal-c/actions/workflows/ci.yml/badge.svg)](https://github.com/YashRao10/dal-c/actions/workflows/ci.yml)
![coverage](https://img.shields.io/badge/coverage-100%25%20line%20%2F%20100%25%20MC%2FDC-38d17a)
![sanitizers](https://img.shields.io/badge/UBSan%20%2B%20ASan-clean-38d17a)
![C99](https://img.shields.io/badge/C-C99%20freestanding-8a6a2c)
![license](https://img.shields.io/badge/license-MIT-8b949e)

> Not affiliated with any airframer or certification authority. This is a
> demonstration of the DO-178C verification *process* applied to real code —
> not certified software.

## Components

| Ref | Module | Purpose | LLR |
|---|---|---|---|
| SC-SAT | `sc_sat` | Saturating 32-bit add / subtract — pins at `INT32_MIN`/`INT32_MAX`, never invokes signed-overflow UB. | 6 |
| SC-FIX | `sc_fixed` | Q16.16 fixed-point: saturating add / sub / mul (round-to-nearest) / div, clamp, abs. | 21 |
| SC-CRC | `sc_crc` | Bitwise CRC-8/SMBUS, CRC-16/CCITT-FALSE, CRC-32/ISO-HDLC. No lookup tables. | 8 |
| SC-HYS | `sc_hysteresis` | Schmitt-trigger comparator with independent assert / clear thresholds. | 12 |
| SC-RB | `sc_ringbuf` | Fixed-capacity byte FIFO over caller storage. No `malloc`, unambiguous full / empty. | 12 |
| SC-RL | `sc_ratelimit` | Slew-rate limiter with output clamp, on saturating arithmetic. | 18 |
| SC-PID | `sc_pid` | Positional PID: derivative-on-measurement + first-order low-pass, output clamp, conditional-integration anti-windup. | 16 |

40 high-level / 93 low-level requirements, **1072** requirements-based test
checks, **100%** statement and **100%** branch/condition (MC/DC) coverage.

Each module is one header in `include/` and one source file in `src/`,
depends only on `<stdint.h>` / `<stdbool.h>` / `<stddef.h>`, and owns no
static state.

## Build

```sh
make            # build + run the test suite (strict warnings, -Werror)
make lib        # -> build/libdal_c.a
make example    # build + run examples/control_loop.c (composed demo)
make coverage   # statement / branch / MC-DC via gcov (GCC >= 14)
make report     # coverage + regenerate docs/traceability.html + docs/verification.html
make sanitize   # suite + example under -fsanitize=undefined,address
make install PREFIX=/usr/local
```

No dependencies beyond a C99 compiler. `make coverage` needs GCC ≥ 14 for
condition (MC/DC) coverage; `make report` also needs Python 3.

## Using it

```c
#include <dal_c/dal_c.h>          /* umbrella, or one component header */

sc_ratelimit_config_t cfg = { .max_step_up = 10, .max_step_down = 10,
                              .out_min = 0, .out_max = 1000 };
sc_ratelimit_state_t  st;
sc_ratelimit_init(&st, 0);
int32_t out = sc_ratelimit_update(&cfg, &st, target);
```

`examples/control_loop.c` composes `sc_pid` + `sc_ratelimit` +
`sc_hysteresis` + `sc_ringbuf` + `sc_fixed` + `sc_crc` into a simulated
room-temperature loop.

## How each component is built

1. **Requirements** — `requirements/<module>.md`: HLRs, then LLRs that each
   trace up to one or more HLRs and name the implementing function.
2. **Code** — every implementation block carries an `/* LLR-... */` tag, so
   the trace between requirement and source is bidirectional.
3. **Requirements-based tests** — `tests/test_<module>.c`; each test names
   the requirement it verifies. Compound decisions get cases chosen so each
   condition is shown to independently affect the outcome (MC/DC).
4. **Structural coverage** — CI runs `make coverage` and fails below 100%
   line and 100% branch / condition on `src/`.
5. **Traceability** — `tools/gen_rtm.py` regenerates
   [`docs/traceability.html`](docs/traceability.html) and fails on any
   orphan requirement, unimplemented LLR, or untested HLR.
6. **Static analysis** — `cppcheck` plus the MISRA C:2012 addon (advisory);
   the suite and example run under UBSan + ASan; the build is exercised on
   `gcc`, `clang`, and `-m32`.

The coverage report, traceability matrix, and verification results are
published to GitHub Pages by CI.

## Coding style

DAL-C-flavoured, not a certified process:

- No dynamic memory, no recursion, no unbounded loops.
- Single point of exit per function where practical.
- All `if … else if` chains terminate in `else` (MISRA C:2012 Rule 15.7).
- Fixed-width integer types; overflow checks arranged so no intermediate
  can itself overflow.
- MISRA C:2012 is the target standard. The one deliberate deviation
  (`__attribute__((warn_unused_result))`, Rule 1.2) is documented at its
  definition in `include/sc_common.h`.

## Layout

```
include/      public headers (one per component + sc_common.h + dal_c.h)
src/          implementations, each block tagged with its LLR
requirements/ per-component HLR/LLR specs
tests/        dependency-free harness + one requirements-based suite per component
examples/     control_loop.c — composed worked example
tools/        gen_rtm.py, gen_report.py — traceability + verification generators
docs/         GitHub Pages site (landing + generated RTM + coverage + results)
```

## License

MIT — see [LICENSE](LICENSE).
