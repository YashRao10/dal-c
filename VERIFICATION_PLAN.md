# dal-c — Software Verification Plan (condensed)

This is a demonstration of the DO-178C verification *process*, not a
certified plan. It records what is verified, how, with which tools, and the
pass criteria the CI pipeline enforces.

## 1. Objectives

For every component:

| # | Objective | Method | Evidence |
|---|-----------|--------|----------|
| O1 | Every high-level requirement is complete and traced to low-level requirements. | Review + generated matrix | `docs/traceability.html` |
| O2 | Every low-level requirement is implemented. | `/* LLR-... */` source tags cross-checked by the matrix generator | `tools/gen_rtm.py` (fails the build on a gap) |
| O3 | Every requirement is verified by test. | Requirements-based tests, each naming its requirement | `tests/test_<module>.c`, `docs/verification.html` |
| O4 | Statement coverage of the source is 100%. | GCC `--coverage`, gcov | `docs/coverage/` (CI fails < 100%) |
| O5 | Modified Condition/Decision Coverage is 100%. | GCC `-fcondition-coverage`, gcov `--conditions` | `docs/coverage/` (CI fails < 100%) |
| O6 | No undefined behaviour is executed. | Suite + example + fuzz under `-fsanitize=undefined,address` | CI `sanitize` job |
| O7 | Invariants hold for arbitrary input, not just the chosen cases. | 200k-iteration randomised property harness | `tests/fuzz.c`, CI |
| O8 | The code conforms to the coding standard. | cppcheck (gate) + MISRA C:2012 addon (advisory) | CI `verify` job, `docs/misra.md` |
| O9 | The library builds on more than one toolchain. | gcc, clang, and `gcc -m32` | CI `portability` job |

## 2. Test method

- **Requirements-based.** Each test function names the HLR/LLR it exercises
  in a comment. For a compound decision, cases are chosen so each condition
  is shown to independently affect the outcome (unique-cause MC/DC where the
  conditions are independent, masking MC/DC where short-circuit `&&`/`||`
  makes a combination unreachable).
- **Property-based.** `tests/fuzz.c` drives `sc_sat`, `sc_ratelimit`,
  `sc_pid`, `sc_ringbuf`, `sc_cobs`, `sc_hysteresis` and `sc_median` with a
  deterministic PRNG and asserts input-independent invariants (output always
  clamped, FIFO order preserved, encode/decode round-trips, the median is
  always a windowed sample, arithmetic never wraps).
- **Structural coverage** is measured on an `-O0` instrumented build so the
  mapping from object to source is exact.

## 3. Tools

| Tool | Use | Qualification note |
|---|---|---|
| GCC ≥ 14 | compiler, coverage instrumentation | not qualified; a real programme would need tool qualification data for the coverage tool per DO-330 |
| gcov | coverage measurement | as above |
| cppcheck | static analysis | advisory here |
| `tools/gen_rtm.py`, `gen_report.py`, `gen_coverage.py` | evidence generation | plain-text transforms of compiler output; low tool-impact |

## 4. Pass criteria (enforced by CI)

The `main` branch is green only if, on `ubuntu-24.04` with `gcc-14`:

1. `make` compiles with `-Werror` and the full strict warning set and the
   suite reports 0 failures.
2. `make fuzz` reports 0 invariant failures.
3. `make report` shows 100% statement and 100% branch/condition coverage on
   `src/` and no traceability gap.
4. `cppcheck` reports no `warning`/`style`/`performance`/`portability` finding.
5. `make sanitize` (suite + example + fuzz under UBSan/ASan) exits clean.

The `portability` job (clang, `-m32`) runs alongside but does not gate the
Pages deploy until it has been confirmed green once.

## 5. What is deliberately out of scope

Object-code coverage, worst-case execution time analysis, a formal Software
Verification Cases and Procedures document, independence of verification
from development, and tool qualification. This project shows the shape of
the process on real code, not a certifiable data package.
