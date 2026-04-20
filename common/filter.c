#include "common/filter.h"

void filter_ema_u16_init(ema_filter_u16_t *filter, uint8_t alpha_q8)
{
    filter->value_q8 = 0U;
    filter->alpha_q8 = alpha_q8;
    filter->initialized = false;
}

uint16_t filter_ema_u16_apply(ema_filter_u16_t *filter, uint16_t sample)
{
    uint32_t sample_q8 = ((uint32_t)sample) << 8U;

    if(!filter->initialized) {
        filter->value_q8 = sample_q8;
        filter->initialized = true;
    } else {
        filter->value_q8 += ((sample_q8 - filter->value_q8) * filter->alpha_q8) >> 8U;
    }

    return (uint16_t)(filter->value_q8 >> 8U);
}
