# sc_median — Requirements

Sliding-window median filter for a noisy analogue input. Rejects isolated
spikes (impulse noise) that a linear filter would smear: the output is the
middle sample of the most recent `length` samples, so a single outlier can
never appear at the output once the window holds three or more samples.

Verified by: `tests/test_sc_median.c`

## High-Level Requirements

- **HLR-MEDIAN-1** — `sc_median_config_valid` shall accept a `length` that
  is odd and in `1..SC_MEDIAN_MAX_WINDOW`, and shall reject a NULL config,
  an even `length`, a `length` of 0, or a `length` greater than
  `SC_MEDIAN_MAX_WINDOW`.
- **HLR-MEDIAN-2** — `sc_median_init` shall place the filter in a known
  empty state and reject a NULL state. `sc_median_update` shall return 0
  for a NULL config or state without modifying the state.
- **HLR-MEDIAN-3** — `sc_median_update` shall maintain a sliding window of
  the most recent `length` samples, overwriting the oldest sample once the
  window is full.
- **HLR-MEDIAN-4** — `sc_median_update` shall return the median of the
  samples currently held in the window. Before the window has filled, it
  shall return the median of the samples seen so far.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-MEDIAN-1 | HLR-MEDIAN-2 | `sc_median_init` clears every `history` slot to 0 and sets `count` and `head` to 0. | `sc_median_init` |
| LLR-MEDIAN-2 | HLR-MEDIAN-2 | `sc_median_init` returns `SC_ERR_NULL` for a NULL `state`, `SC_OK` otherwise. | `sc_median_init` |
| LLR-MEDIAN-3 | HLR-MEDIAN-1 | `sc_median_config_valid` returns `SC_ERR_NULL` for a NULL `cfg`. | `sc_median_config_valid` |
| LLR-MEDIAN-4 | HLR-MEDIAN-1 | Returns `SC_ERR_PARAM` when `length` exceeds `SC_MEDIAN_MAX_WINDOW`. | `sc_median_config_valid` |
| LLR-MEDIAN-5 | HLR-MEDIAN-1 | Returns `SC_ERR_PARAM` when `length` is even (which includes 0). | `sc_median_config_valid` |
| LLR-MEDIAN-6 | HLR-MEDIAN-1 | Returns `SC_OK` for an odd `length` within range. | `sc_median_config_valid` |
| LLR-MEDIAN-7 | HLR-MEDIAN-2 | `sc_median_update` returns 0 and does not touch the state when `cfg` or `state` is NULL. | `sc_median_update` |
| LLR-MEDIAN-8 | HLR-MEDIAN-3 | `sc_median_update` writes the new sample to `history[head]` and advances `head` modulo `length`. | `sc_median_update` |
| LLR-MEDIAN-9 | HLR-MEDIAN-3 | `count` increases by one per update until it reaches `length`, then holds. | `sc_median_update` |
| LLR-MEDIAN-10 | HLR-MEDIAN-4 | `sc_median_update` copies the `count` live samples into a scratch buffer and sorts them ascending with an insertion sort. | `sc_median_update` |
| LLR-MEDIAN-11 | HLR-MEDIAN-4 | `sc_median_update` returns the scratch element at index `count / 2`. | `sc_median_update` |
