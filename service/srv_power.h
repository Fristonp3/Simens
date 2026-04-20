#ifndef SRV_POWER_H
#define SRV_POWER_H

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"

void srv_power_init(uint32_t now_ms);
void srv_power_mark_activity(uint32_t now_ms);
void srv_power_wake(uint32_t now_ms);
bool srv_power_should_enter_sleep(uint32_t now_ms, app_state_t state);
uint32_t srv_power_last_activity(void);

#endif
