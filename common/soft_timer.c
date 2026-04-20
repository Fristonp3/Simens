#include "common/soft_timer.h"

static bool soft_timer_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms)) >= 0;
}

void soft_timer_stop(soft_timer_t *timer)
{
    timer->active = false;
    timer->periodic = false;
    timer->interval_ms = 0U;
    timer->deadline_ms = 0U;
}

void soft_timer_start_periodic(soft_timer_t *timer, uint32_t now_ms, uint32_t interval_ms)
{
    timer->active = true;
    timer->periodic = true;
    timer->interval_ms = interval_ms;
    timer->deadline_ms = now_ms + interval_ms;
}

void soft_timer_start_oneshot(soft_timer_t *timer, uint32_t now_ms, uint32_t interval_ms)
{
    timer->active = true;
    timer->periodic = false;
    timer->interval_ms = interval_ms;
    timer->deadline_ms = now_ms + interval_ms;
}

bool soft_timer_expired(soft_timer_t *timer, uint32_t now_ms)
{
    if((!timer->active) || (!soft_timer_reached(now_ms, timer->deadline_ms))) {
        return false;
    }

    if(timer->periodic) {
        timer->deadline_ms += timer->interval_ms;
    } else {
        timer->active = false;
    }

    return true;
}

uint32_t soft_timer_remaining(const soft_timer_t *timer, uint32_t now_ms)
{
    if(!timer->active) {
        return 0U;
    }

    if(soft_timer_reached(now_ms, timer->deadline_ms)) {
        return 0U;
    }

    return timer->deadline_ms - now_ms;
}
