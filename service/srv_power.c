#include "service/srv_power.h"

static uint32_t s_last_activity_ms;

void srv_power_init(uint32_t now_ms)
{
    s_last_activity_ms = now_ms;
}

void srv_power_mark_activity(uint32_t now_ms)
{
    s_last_activity_ms = now_ms;
}

void srv_power_wake(uint32_t now_ms)
{
    s_last_activity_ms = now_ms;
}

bool srv_power_should_enter_sleep(uint32_t now_ms, app_state_t state)
{
    if((state == APP_STATE_INIT) || (state == APP_STATE_RECORD) || (state == APP_STATE_BOOT)) {
        return false;
    }

    return ((now_ms - s_last_activity_ms) >= PROJECT_IDLE_SLEEP_TIMEOUT_MS);
}

uint32_t srv_power_last_activity(void)
{
    return s_last_activity_ms;
}
