# sc_vote — Requirements

Redundancy voter for replicated sensor or channel readings. Given `count`
independent readings of the same quantity, it groups them into agreement
clusters — two readings agree when they differ by no more than `tolerance` —
and reports the reading backed by the largest cluster, how many channels
stood behind it, which channels dissented, and whether the cluster was large
enough to be trusted.

Stateless: the vote is a pure function of the current readings and the
configuration. No history, no dynamic memory; the working arrays are sized
by `SC_VOTE_MAX_INPUTS` and every loop is bounded by it.

Verified by: `tests/test_sc_vote.c`

## High-Level Requirements

- **HLR-VOTE-1** — `sc_vote_config_valid` shall accept a `count` in
  `2..SC_VOTE_MAX_INPUTS`, an `agree` in `1..count`, and a non-negative
  `tolerance`, and shall reject a NULL config, a `count` outside that
  range, an `agree` of 0 or greater than `count`, or a negative
  `tolerance`.
- **HLR-VOTE-2** — `sc_vote_evaluate` shall return `SC_ERR_NULL` if any of
  `cfg`, `inputs`, or `out` is NULL, and `SC_ERR_PARAM` if `n` is not equal
  to `cfg->count`, in either case without producing a result.
- **HLR-VOTE-3** — `sc_vote_evaluate` shall compute, for each channel, the
  number of channels whose reading is within `tolerance` of it, select the
  channel with the largest such count (earliest channel on a tie) as the
  winner, and set `out->value` to the winner's reading and `out->agreeing`
  to the winner's count. The within-tolerance test shall not overflow for
  any `int32_t` readings.
- **HLR-VOTE-4** — `sc_vote_evaluate` shall set `out->verdict` to
  `SC_VOTE_OK` when `out->agreeing` is at least `cfg->agree`, and to
  `SC_VOTE_NO_CONSENSUS` otherwise, and shall return `SC_OK` once a result
  has been written for either verdict.
- **HLR-VOTE-5** — `sc_vote_evaluate` shall set bit `i` of
  `out->dissenting` when channel `i`'s reading is not within `tolerance` of
  `out->value`, and clear every other bit.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-VOTE-1 | HLR-VOTE-1 | `sc_vote_config_valid` returns `SC_ERR_NULL` for a NULL `cfg`. | `sc_vote_config_valid` |
| LLR-VOTE-2 | HLR-VOTE-1 | Returns `SC_ERR_PARAM` when `count` is below 2 or above `SC_VOTE_MAX_INPUTS`. | `sc_vote_config_valid` |
| LLR-VOTE-3 | HLR-VOTE-1 | Returns `SC_ERR_PARAM` when `agree` is 0 or greater than `count`. | `sc_vote_config_valid` |
| LLR-VOTE-4 | HLR-VOTE-1 | Returns `SC_ERR_PARAM` when `tolerance` is negative. | `sc_vote_config_valid` |
| LLR-VOTE-5 | HLR-VOTE-1 | Returns `SC_OK` when `cfg` is non-NULL and `count`, `agree` and `tolerance` are all in range. | `sc_vote_config_valid` |
| LLR-VOTE-6 | HLR-VOTE-2 | `sc_vote_evaluate` returns `SC_ERR_NULL` if `cfg`, `inputs` or `out` is NULL. | `sc_vote_evaluate` |
| LLR-VOTE-7 | HLR-VOTE-2 | `sc_vote_evaluate` returns `SC_ERR_PARAM` if `n` is not equal to `cfg->count`. | `sc_vote_evaluate` |
| LLR-VOTE-8 | HLR-VOTE-3 | For each channel `i`, computes its cluster size as the number of channels `j` (`0 <= j < n`) for which `within(inputs[i], inputs[j], tolerance)` holds. | `sc_vote_evaluate` |
| LLR-VOTE-9 | HLR-VOTE-3 | `within(a, b, tol)` forms `a - b` widened to 64-bit, negates it when negative, and returns whether the magnitude does not exceed `tol`; the widening admits no intermediate overflow. | `within` |
| LLR-VOTE-10 | HLR-VOTE-3 | Keeps `winner` as the channel with the greatest cluster size seen so far, leaving it unchanged when a later channel only ties. | `sc_vote_evaluate` |
| LLR-VOTE-11 | HLR-VOTE-3 | Sets `out->value` to `inputs[winner]` and `out->agreeing` to `sizes[winner]`. | `sc_vote_evaluate` |
| LLR-VOTE-12 | HLR-VOTE-4 | Sets `out->verdict` to `SC_VOTE_OK` when `out->agreeing >= cfg->agree`, otherwise `SC_VOTE_NO_CONSENSUS`. | `sc_vote_evaluate` |
| LLR-VOTE-13 | HLR-VOTE-5 | Sets bit `i` of `out->dissenting` when `within(inputs[i], out->value, tolerance)` is false, and leaves every other bit clear. | `sc_vote_evaluate` |
| LLR-VOTE-14 | HLR-VOTE-4 | Returns `SC_OK` once `out` has been fully populated. | `sc_vote_evaluate` |
