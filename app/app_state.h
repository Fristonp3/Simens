#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "app/app_main.h"

void app_state_init(app_context_t *ctx, uint32_t now_ms);
bool app_state_set(app_context_t *ctx, app_state_t state, uint32_t now_ms);
bool app_state_set_by_mode_id(app_context_t *ctx, uint8_t mode_id, uint32_t now_ms);
void app_state_process_keys(app_context_t *ctx, uint32_t now_ms);
void app_state_update_leds(const app_context_t *ctx, uint32_t now_ms);
const char *app_state_name(app_state_t state);

#endif
