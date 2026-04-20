#include "common/ringbuffer.h"

void ringbuffer_init(ringbuffer_t *rb, uint8_t *storage, uint16_t size)
{
    rb->buffer = storage;
    rb->size = size;
    rb->head = 0U;
    rb->tail = 0U;
}

void ringbuffer_reset(ringbuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
}

bool ringbuffer_push(ringbuffer_t *rb, uint8_t value)
{
    uint16_t next = (uint16_t)((rb->head + 1U) % rb->size);

    if(next == rb->tail) {
        return false;
    }

    rb->buffer[rb->head] = value;
    rb->head = next;
    return true;
}

bool ringbuffer_pop(ringbuffer_t *rb, uint8_t *value)
{
    if(rb->head == rb->tail) {
        return false;
    }

    *value = rb->buffer[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) % rb->size);
    return true;
}

uint16_t ringbuffer_count(const ringbuffer_t *rb)
{
    if(rb->head >= rb->tail) {
        return (uint16_t)(rb->head - rb->tail);
    }

    return (uint16_t)(rb->size - rb->tail + rb->head);
}
