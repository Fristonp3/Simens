#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"
#include "common/soft_timer.h"

typedef struct {
    system_config_t config;
    system_status_t status;
    sample_record_t latest_sample;
    uint16_t replay_index;
    uint8_t config_cursor;
    uint32_t last_uart_ms;
    bool display_dirty;
    soft_timer_t sample_timer;
    soft_timer_t display_timer;
    soft_timer_t record_timer;
} app_context_t;

void app_main_init(void);
void app_main_run(void);
void app_main_process(void);

#endif
