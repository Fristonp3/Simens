#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool active;
    bool periodic;
    uint32_t interval_ms;
    uint32_t deadline_ms;
} soft_timer_t;

void soft_timer_stop(soft_timer_t *timer);
void soft_timer_start_periodic(soft_timer_t *timer, uint32_t now_ms, uint32_t interval_ms);
void soft_timer_start_oneshot(soft_timer_t *timer, uint32_t now_ms, uint32_t interval_ms);
bool soft_timer_expired(soft_timer_t *timer, uint32_t now_ms);
uint32_t soft_timer_remaining(const soft_timer_t *timer, uint32_t now_ms);

#endif
