/*
 * sc_ringbuf.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_ringbuf.md.
 */
#include "sc_ringbuf.h"

sc_status_t sc_ringbuf_init(sc_ringbuf_t *rb, uint8_t *storage, uint32_t capacity)
{
    sc_status_t status;

    if ((rb == NULL) || (storage == NULL))
    {
        status = SC_ERR_NULL;                 /* LLR-RB-9  */
    }
    else if (capacity == 0u)
    {
        status = SC_ERR_PARAM;                /* LLR-RB-10 */
    }
    else
    {
        rb->buffer   = storage;              /* LLR-RB-1  */
        rb->capacity = capacity;
        rb->head     = 0u;
        rb->tail     = 0u;
        rb->count    = 0u;
        status = SC_OK;
    }

    return status;
}

bool sc_ringbuf_is_empty(const sc_ringbuf_t *rb)
{
    bool result;

    if (rb == NULL)
    {
        result = true;                       /* LLR-RB-11 */
    }
    else
    {
        result = (rb->count == 0u);          /* LLR-RB-2  */
    }

    return result;
}

bool sc_ringbuf_is_full(const sc_ringbuf_t *rb)
{
    bool result;

    if (rb == NULL)
    {
        result = false;                      /* LLR-RB-11 */
    }
    else
    {
        result = (rb->count == rb->capacity); /* LLR-RB-3 */
    }

    return result;
}

uint32_t sc_ringbuf_count(const sc_ringbuf_t *rb)
{
    uint32_t result;

    if (rb == NULL)
    {
        result = 0u;                         /* LLR-RB-11 */
    }
    else
    {
        result = rb->count;                  /* LLR-RB-4  */
    }

    return result;
}

sc_status_t sc_ringbuf_push(sc_ringbuf_t *rb, uint8_t value)
{
    sc_status_t status;

    if (rb == NULL)
    {
        status = SC_ERR_NULL;                 /* LLR-RB-9  */
    }
    else if (rb->count == rb->capacity)
    {
        status = SC_ERR_FULL;                 /* LLR-RB-6  */
    }
    else
    {
        rb->buffer[rb->head] = value;        /* LLR-RB-5  */
        rb->head++;
        if (rb->head == rb->capacity)        /* LLR-RB-7: wrap */
        {
            rb->head = 0u;
        }
        rb->count++;
        status = SC_OK;
    }

    return status;
}

sc_status_t sc_ringbuf_pop(sc_ringbuf_t *rb, uint8_t *out)
{
    sc_status_t status;

    if ((rb == NULL) || (out == NULL))
    {
        status = SC_ERR_NULL;                 /* LLR-RB-9  */
    }
    else if (rb->count == 0u)
    {
        status = SC_ERR_EMPTY;               /* LLR-RB-8  */
    }
    else
    {
        *out = rb->buffer[rb->tail];         /* LLR-RB-5  */
        rb->tail++;
        if (rb->tail == rb->capacity)        /* LLR-RB-7: wrap */
        {
            rb->tail = 0u;
        }
        rb->count--;
        status = SC_OK;
    }

    return status;
}

sc_status_t sc_ringbuf_reset(sc_ringbuf_t *rb)
{
    sc_status_t status;

    if (rb == NULL)
    {
        status = SC_ERR_NULL;                 /* LLR-RB-9  */
    }
    else
    {
        rb->head  = 0u;                      /* LLR-RB-12 */
        rb->tail  = 0u;
        rb->count = 0u;
        status = SC_OK;
    }

    return status;
}
