# sc_cobs — Requirements

Consistent Overhead Byte Stuffing: encode a byte string so it contains no
`0x00`, letting `0x00` delimit frames.

Verified by: `tests/test_sc_cobs.c`

## High-Level Requirements

- **HLR-COBS-1** — `sc_cobs_max_encoded(n)` shall return an upper bound on
  the encoded length of any `n`-byte payload.
- **HLR-COBS-2** — `sc_cobs_encode` shall produce output containing no
  `0x00` byte, and shall return 0 if an argument is NULL or the output
  would not fit.
- **HLR-COBS-3** — `sc_cobs_decode` shall be the exact inverse of
  `sc_cobs_encode` for every payload: `decode(encode(p)) == p`.
- **HLR-COBS-4** — `sc_cobs_decode` shall return 0 for malformed input (a
  `0x00` where a code byte is expected, or a block length that runs past
  the end) or when the output would not fit.

## Low-Level Requirements

| ID | Traces | Statement | Code |
|----|--------|-----------|------|
| LLR-COBS-1  | HLR-COBS-1 | `sc_cobs_max_encoded` returns 1 for `n == 0`, else `n + (n-1)/254 + 1`. | `sc_cobs_max_encoded` |
| LLR-COBS-2  | HLR-COBS-2 | `sc_cobs_encode` returns 0 when `in` or `out` is NULL or `sc_cobs_max_encoded(in_len) > out_cap`. | `sc_cobs_encode` |
| LLR-COBS-3  | HLR-COBS-2, HLR-COBS-3 | Encode consumes the input one byte at a time. | `sc_cobs_encode` |
| LLR-COBS-4  | HLR-COBS-2 | On a `0x00` input byte, the current code is written and a new block started. | `sc_cobs_encode` |
| LLR-COBS-5  | HLR-COBS-2 | A non-zero input byte is copied to the output and the code incremented. | `sc_cobs_encode` |
| LLR-COBS-6  | HLR-COBS-2 | When the code reaches `0xFF` (254 data bytes) and more input remains, the block is closed with `0xFF`. | `sc_cobs_encode` |
| LLR-COBS-7  | HLR-COBS-2 | After the input is consumed, the pending code is written into its reserved slot. | `sc_cobs_encode` |
| LLR-COBS-8  | HLR-COBS-4 | `sc_cobs_decode` returns 0 when `in` or `out` is NULL. | `sc_cobs_decode` |
| LLR-COBS-9  | HLR-COBS-3, HLR-COBS-4 | Decode reads a code byte, then that many minus one data bytes, repeating. | `sc_cobs_decode` |
| LLR-COBS-10 | HLR-COBS-4 | A code byte of `0x00`, or a span that exceeds the remaining input or output, sets the failure flag. | `sc_cobs_decode` |
| LLR-COBS-11 | HLR-COBS-3 | `code - 1` data bytes are copied verbatim from input to output. | `sc_cobs_decode` |
| LLR-COBS-12 | HLR-COBS-3 | Unless the code was `0xFF` or the input is exhausted, a single `0x00` is appended between blocks. | `sc_cobs_decode` |
| LLR-COBS-13 | HLR-COBS-4 | Decode returns the decoded length on success, 0 on any failure. | `sc_cobs_decode` |

## Design notes

Encode uses lazy block splitting: a full 254-byte block is only closed
with a `0xFF` code when a byte follows it, so exactly 254 non-zero bytes
encode to 255 (not 256). Both functions bound every write against the
supplied capacity; the bounds tests are written as subtractions so no
intermediate can overflow. Precondition on `sc_cobs_decode`: the input
must be one frame with no `0x00` bytes (the delimiter is handled by the
caller).
