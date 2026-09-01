# dal-c — MISRA C:2012 conformance and deviations

MISRA C:2012 is the coding-standard target. `cppcheck --addon=misra` runs in
CI as an advisory check. This file records the deviations the code takes,
with justification — the form a real project would keep, scaled down.

## Deviations

### D1 — Rule 1.2 (language extensions): `__attribute__((warn_unused_result))`

**Where:** `include/sc_common.h`, macro `SC_NODISCARD`, applied to every
function returning `sc_status_t`.

**Deviation:** the GCC/Clang attribute is a language extension.

**Justification:** the attribute has no effect on the translated program —
it only causes the compiler to warn when a status return is discarded, which
is a defect this library treats as important (a dropped `SC_ERR_FULL` from
`sc_ringbuf_push`, say). It is guarded by `#if defined(__GNUC__)` and
compiles to nothing elsewhere, so portability is unaffected.

**Scope:** one macro definition; the risk is bounded and understood.

### D2 — Rule 21.x / Dir 4.6 (standard library, fixed-width types)

**Where:** every header includes `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`.

**Deviation / interpretation:** MISRA restricts parts of the standard
library and, under Dir 4.6, discourages the plain types. This project uses
only the three headers above, only for `int32_t`/`uint32_t`/`uint16_t`/
`uint8_t`/`bool`/`NULL`/`INT32_MIN`/`INT32_MAX`. Directive 4.6 explicitly
permits fixed-width types; `<stdbool.h>` and `<stddef.h>` are standard and
have no unsafe surface used here.

**Justification:** using fixed-width types is *required* for the
determinism this library targets; avoiding them would be worse.

## Advisory findings not deviated

`cppcheck --addon=misra` may still report advisory-category rules (e.g.
Rule 15.5 single-exit as an advisory, or Rule 10.x essential-type notes on
the deliberate `== true` / `== false` comparisons that make the MC/DC
obligations explicit). These are reviewed case by case in the CI log and
are not treated as build failures; none currently require a code change or
a formal deviation.

## Enforced without deviation

- No dynamic memory (`malloc`/`free` absent from `src/`).
- No recursion.
- No unbounded loops — every loop bound is a compile-time or
  argument-supplied count.
- All `if … else if` chains end in `else` (Rule 15.7).
- No implicit narrowing — `-Wconversion` is part of the `-Werror` set.
