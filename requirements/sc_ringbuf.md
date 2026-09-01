# sc_ringbuf — Requirements

A fixed-capacity byte FIFO over caller-provided storage. No dynamic memory.

Verified by: `tests/test_sc_ringbuf.c`

## High-Level Requirements

- **HLR-RB-1** — `sc_ringbuf_init` shall bind the buffer to caller storage
  of a stated capacity and leave it empty; it shall reject a NULL buffer,
  NULL storage, or zero capacity.
- **HLR-RB-2** — Bytes shall be returned by `sc_ringbuf_pop` in the order
  they were supplied to `sc_ringbuf_push` (first-in, first-out).
- **HLR-RB-3** — The buffer shall reuse storage cyclically: after `capacity`
  pushes the write position wraps to the start, and likewise for reads.
- **HLR-RB-4** — `sc_ringbuf_push` shall return `SC_ERR_FULL` and make no
  change when the buffer already holds `capacity` bytes.
- **HLR-RB-5** — `sc_ringbuf_pop` shall return `SC_ERR_EMPTY` and leave the
  output argument unchanged when the buffer holds no bytes.
- **HLR-RB-6** — `sc_ringbuf_reset` shall discard all buffered bytes while
  keeping the storage binding usable.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-RB-1  | HLR-RB-1 | `sc_ringbuf_init` stores `storage`, `capacity`, and zeroes `head`, `tail`, `count`. | `sc_ringbuf_init` |
| LLR-RB-2  | HLR-RB-1, HLR-RB-5 | `sc_ringbuf_is_empty` returns `count == 0`. | `sc_ringbuf_is_empty` |
| LLR-RB-3  | HLR-RB-4 | `sc_ringbuf_is_full` returns `count == capacity`. | `sc_ringbuf_is_full` |
| LLR-RB-4  | HLR-RB-2 | `sc_ringbuf_count` returns `count`. | `sc_ringbuf_count` |
| LLR-RB-5  | HLR-RB-2 | `sc_ringbuf_push` writes `value` at `head`; `sc_ringbuf_pop` reads from `tail` into `*out`. | `sc_ringbuf_push`, `sc_ringbuf_pop` |
| LLR-RB-6  | HLR-RB-4 | `sc_ringbuf_push` returns `SC_ERR_FULL` without writing when `count == capacity`. | `sc_ringbuf_push` |
| LLR-RB-7  | HLR-RB-3 | After advancing, `head` / `tail` wrap to 0 when they reach `capacity`; `count` is incremented on push and decremented on pop. | `sc_ringbuf_push`, `sc_ringbuf_pop` |
| LLR-RB-8  | HLR-RB-5 | `sc_ringbuf_pop` returns `SC_ERR_EMPTY` without touching `*out` when `count == 0`. | `sc_ringbuf_pop` |
| LLR-RB-9  | HLR-RB-1, HLR-RB-5 | Every mutating entry point returns `SC_ERR_NULL` when a required pointer (`rb`, `storage`, `out`) is NULL. | all |
| LLR-RB-10 | HLR-RB-1 | `sc_ringbuf_init` returns `SC_ERR_PARAM` when `capacity == 0`. | `sc_ringbuf_init` |
| LLR-RB-11 | HLR-RB-1 | The query helpers treat a NULL `rb` as empty / not-full / count 0. | `sc_ringbuf_is_empty`, `sc_ringbuf_is_full`, `sc_ringbuf_count` |
| LLR-RB-12 | HLR-RB-6 | `sc_ringbuf_reset` zeroes `head`, `tail`, `count` and returns `SC_OK`. | `sc_ringbuf_reset` |

## Design notes

A distinct `count` field (rather than the head/tail-only scheme) keeps the
full and empty states unambiguous and removes any power-of-two capacity
requirement. Wrap is `if (index == capacity) index = 0;` — no `%` operator,
which some coding standards restrict.
