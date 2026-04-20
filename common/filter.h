#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t value_q8;
    uint8_t alpha_q8;
    bool initialized;
} ema_filter_u16_t;

void filter_ema_u16_init(ema_filter_u16_t *filter, uint8_t alpha_q8);
uint16_t filter_ema_u16_apply(ema_filter_u16_t *filter, uint16_t sample);

#endif
