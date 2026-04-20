#ifndef APP_STORAGE_H
#define APP_STORAGE_H

#include <stdbool.h>

#include "app/app_main.h"

bool app_storage_append_latest(app_context_t *ctx);
bool app_storage_get_replay_record(const app_context_t *ctx, sample_record_t *record);
void app_storage_next_record(app_context_t *ctx);
void app_storage_previous_record(app_context_t *ctx);
void app_storage_clear(app_context_t *ctx);

#endif
