#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    uint16_t size;
    volatile uint16_t head;
    volatile uint16_t tail;
} ringbuffer_t;

void ringbuffer_init(ringbuffer_t *rb, uint8_t *storage, uint16_t size);
void ringbuffer_reset(ringbuffer_t *rb);
bool ringbuffer_push(ringbuffer_t *rb, uint8_t value);
bool ringbuffer_pop(ringbuffer_t *rb, uint8_t *value);
uint16_t ringbuffer_count(const ringbuffer_t *rb);

#endif
