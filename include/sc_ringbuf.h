/*
 * sc_ringbuf.h -- fixed-capacity byte ring buffer over caller-provided storage.
 *
 * No dynamic allocation: the caller supplies the backing array and its
 * length to sc_ringbuf_init(). A separate element count is tracked so the
 * full and empty states are unambiguous and the capacity need not be a
 * power of two.
 *
 * Concurrency: safe for one producer calling _push and one consumer calling
 * _pop without a lock ONLY where aligned 32-bit loads/stores are atomic and
 * ordered with respect to the data byte. Otherwise serialise externally.
 * The single-threaded contract is always safe.
 */
#ifndef DAL_C_SC_RINGBUF_H
#define DAL_C_SC_RINGBUF_H

#include "sc_common.h"

typedef struct
{
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t head;   /* next write index */
    uint32_t tail;   /* next read index  */
    uint32_t count;  /* elements in use  */
} sc_ringbuf_t;

/*
 * Bind a ring buffer to storage of `capacity` bytes (>= 1).
 *   SC_OK / SC_ERR_NULL (rb or storage NULL) / SC_ERR_PARAM (capacity 0)
 */
SC_NODISCARD sc_status_t sc_ringbuf_init(sc_ringbuf_t *rb,
                                         uint8_t *storage,
                                         uint32_t capacity);

/* Query helpers. A NULL rb reads as empty, not-full, count 0. */
bool     sc_ringbuf_is_empty(const sc_ringbuf_t *rb);
bool     sc_ringbuf_is_full(const sc_ringbuf_t *rb);
uint32_t sc_ringbuf_count(const sc_ringbuf_t *rb);

/* Append one byte.  SC_OK / SC_ERR_NULL / SC_ERR_FULL */
SC_NODISCARD sc_status_t sc_ringbuf_push(sc_ringbuf_t *rb, uint8_t value);

/* Remove the oldest byte into *out.  SC_OK / SC_ERR_NULL / SC_ERR_EMPTY */
SC_NODISCARD sc_status_t sc_ringbuf_pop(sc_ringbuf_t *rb, uint8_t *out);

/* Discard all buffered data; keeps the storage binding.  SC_OK / SC_ERR_NULL */
SC_NODISCARD sc_status_t sc_ringbuf_reset(sc_ringbuf_t *rb);

#endif /* DAL_C_SC_RINGBUF_H */
