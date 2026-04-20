#include "app/app_state.h"

#include "app/app_storage.h"
#include "bootloader/bl_main.h"
#include "bsp/bsp_dac.h"
#include "bsp/bsp_gpio.h"
#include "service/srv_power.h"
#include "service/srv_settings.h"

static void app_state_adjust_config(app_context_t *ctx, uint32_t now_ms)
{
    switch(ctx->config_cursor) {
    case 0U:
        ctx->config.sample_period_ms += 100U;
        if(ctx->config.sample_period_ms > 1000U) {
            ctx->config.sample_period_ms = 100U;
        }
        soft_timer_start_periodic(&ctx->sample_timer, now_ms, ctx->config.sample_period_ms);
        break;

    case 1U:
        ctx->config.display_period_ms += 100U;
        if(ctx->config.display_period_ms > 1000U) {
            ctx->config.display_period_ms = 100U;
        }
        soft_timer_start_periodic(&ctx->display_timer, now_ms, ctx->config.display_period_ms);
        break;

    case 2U:
        ctx->config.record_period_ms += 500U;
        if(ctx->config.record_period_ms > 5000U) {
            ctx->config.record_period_ms = 500U;
        }
        if(ctx->status.recording_enabled) {
            soft_timer_start_periodic(&ctx->record_timer, now_ms, ctx->config.record_period_ms);
        }
        break;

    case 3U:
    default:
        ctx->config.dac_raw = (uint16_t)(ctx->config.dac_raw + 256U);
        if(ctx->config.dac_raw > 4095U) {
            ctx->config.dac_raw = 0U;
        }
        bsp_dac_set_raw(ctx->config.dac_raw);
        break;
    }

    srv_settings_save(&ctx->config);
    ctx->display_dirty = true;
}

static app_state_t app_state_next_mode(app_state_t current)
{
    switch(current) {
    case APP_STATE_MONITOR:
        return APP_STATE_RECORD;
    case APP_STATE_RECORD:
        return APP_STATE_REPLAY;
    case APP_STATE_REPLAY:
        return APP_STATE_CONFIG;
    case APP_STATE_CONFIG:
    default:
        return APP_STATE_MONITOR;
    }
}

void app_state_init(app_context_t *ctx, uint32_t now_ms)
{
    ctx->status.current_state = APP_STATE_INIT;
    ctx->status.previous_state = APP_STATE_MONITOR;
    ctx->status.recording_enabled = false;
    ctx->status.storage_full = false;
    ctx->status.sleep_armed = false;
    ctx->status.boot_requested = false;
    ctx->config_cursor = 0U;

    (void)app_state_set(ctx, APP_STATE_MONITOR, now_ms);
}

bool app_state_set(app_context_t *ctx, app_state_t state, uint32_t now_ms)
{
    if(state == ctx->status.current_state) {
        return true;
    }

    if(ctx->status.current_state == APP_STATE_RECORD) {
        ctx->status.recording_enabled = false;
        soft_timer_stop(&ctx->record_timer);
    }

    if(state == APP_STATE_SLEEP) {
        ctx->status.previous_state = ctx->status.current_state;
        ctx->status.sleep_armed = true;
    } else if(ctx->status.current_state == APP_STATE_SLEEP) {
        ctx->status.sleep_armed = false;
    }

    if(state == APP_STATE_RECORD) {
        ctx->status.recording_enabled = true;
        soft_timer_start_periodic(&ctx->record_timer, now_ms, ctx->config.record_period_ms);
    }

    if(state == APP_STATE_REPLAY) {
        if(ctx->status.record_count == 0U) {
            ctx->replay_index = 0U;
        } else if(ctx->replay_index >= ctx->status.record_count) {
            ctx->replay_index = (uint16_t)(ctx->status.record_count - 1U);
        }
    }

    if(state == APP_STATE_BOOT) {
        ctx->status.boot_requested = true;
        bl_main_request_enter();
    } else {
        ctx->status.boot_requested = false;
    }

    ctx->status.current_state = state;
    srv_power_mark_activity(now_ms);
    ctx->status.last_activity_ms = now_ms;
    ctx->display_dirty = true;
    return true;
}

bool app_state_set_by_mode_id(app_context_t *ctx, uint8_t mode_id, uint32_t now_ms)
{
    app_state_t target;

    switch(mode_id) {
    case 0U:
    case 1U:
        target = APP_STATE_MONITOR;
        break;
    case 2U:
        target = APP_STATE_RECORD;
        break;
    case 3U:
        target = APP_STATE_REPLAY;
        break;
    case 4U:
        target = APP_STATE_CONFIG;
        break;
    case 5U:
        target = APP_STATE_SLEEP;
        break;
    case 6U:
        target = APP_STATE_BOOT;
        break;
    default:
        return false;
    }

    return app_state_set(ctx, target, now_ms);
}

void app_state_process_keys(app_context_t *ctx, uint32_t now_ms)
{
    key_event_t wake_event = bsp_gpio_take_key_event(BSP_KEY_WAKE);
    key_event_t mode_event = bsp_gpio_take_key_event(BSP_KEY_MODE);
    key_event_t next_event = bsp_gpio_take_key_event(BSP_KEY_NEXT);

    if(wake_event != KEY_EVENT_NONE || mode_event != KEY_EVENT_NONE || next_event != KEY_EVENT_NONE) {
        srv_power_mark_activity(now_ms);
        ctx->status.last_activity_ms = now_ms;
    }

    if(mode_event == KEY_EVENT_SHORT) {
        if(ctx->status.current_state == APP_STATE_SLEEP) {
            (void)app_state_set(ctx, ctx->status.previous_state, now_ms);
        } else if(ctx->status.current_state != APP_STATE_BOOT) {
            (void)app_state_set(ctx, app_state_next_mode(ctx->status.current_state), now_ms);
        }
    } else if(mode_event == KEY_EVENT_LONG) {
        (void)app_state_set(ctx, APP_STATE_BOOT, now_ms);
    }

    if(next_event == KEY_EVENT_SHORT) {
        if(ctx->status.current_state == APP_STATE_CONFIG) {
            app_state_adjust_config(ctx, now_ms);
        } else if(ctx->status.current_state == APP_STATE_REPLAY) {
            app_storage_next_record(ctx);
        } else {
            ctx->config.display_page = (ctx->config.display_page == APP_PAGE_MAIN) ? APP_PAGE_EXTENDED : APP_PAGE_MAIN;
            srv_settings_save(&ctx->config);
            ctx->display_dirty = true;
        }
    } else if(next_event == KEY_EVENT_LONG) {
        if(ctx->status.current_state == APP_STATE_CONFIG) {
            (void)app_state_set(ctx, APP_STATE_MONITOR, now_ms);
        } else {
            ctx->config.display_page = (ctx->config.display_page == APP_PAGE_MAIN) ? APP_PAGE_EXTENDED : APP_PAGE_MAIN;
            srv_settings_save(&ctx->config);
            ctx->display_dirty = true;
        }
    }

    if(wake_event == KEY_EVENT_SHORT) {
        if(ctx->status.current_state == APP_STATE_CONFIG) {
            ctx->config_cursor = (uint8_t)((ctx->config_cursor + 1U) % 4U);
            ctx->display_dirty = true;
        } else if(ctx->status.current_state == APP_STATE_REPLAY) {
            app_storage_previous_record(ctx);
        } else if(ctx->status.current_state == APP_STATE_SLEEP) {
            (void)app_state_set(ctx, ctx->status.previous_state, now_ms);
        } else if(ctx->status.current_state != APP_STATE_BOOT) {
            (void)app_state_set(ctx, APP_STATE_MONITOR, now_ms);
        }
    } else if((wake_event == KEY_EVENT_LONG) &&
              (ctx->status.current_state != APP_STATE_RECORD) &&
              (ctx->status.current_state != APP_STATE_BOOT)) {
        (void)app_state_set(ctx, APP_STATE_SLEEP, now_ms);
    }
}

void app_state_update_leds(const app_context_t *ctx, uint32_t now_ms)
{
    bool slow = (((now_ms / 500U) % 2U) != 0U);
    bool fast = (((now_ms / 125U) % 2U) != 0U);
    bool fault = ((ctx->latest_sample.status & (SAMPLE_STATUS_SENSOR_OPEN | SAMPLE_STATUS_SENSOR_SHORT)) != 0U);

    switch(ctx->status.current_state) {
    case APP_STATE_MONITOR:
        bsp_gpio_set_led(BSP_LED_RUN, slow);
        bsp_gpio_set_led(BSP_LED_RECORD, false);
        bsp_gpio_set_led(BSP_LED_STATUS, fault ? fast : ctx->status.uart_online);
        break;

    case APP_STATE_RECORD:
        bsp_gpio_set_led(BSP_LED_RUN, true);
        bsp_gpio_set_led(BSP_LED_RECORD, fast);
        bsp_gpio_set_led(BSP_LED_STATUS, fault ? fast : ctx->status.uart_online);
        break;

    case APP_STATE_REPLAY:
        bsp_gpio_set_led(BSP_LED_RUN, false);
        bsp_gpio_set_led(BSP_LED_RECORD, slow);
        bsp_gpio_set_led(BSP_LED_STATUS, ctx->status.uart_online);
        break;

    case APP_STATE_CONFIG:
        bsp_gpio_set_led(BSP_LED_RUN, slow);
        bsp_gpio_set_led(BSP_LED_RECORD, true);
        bsp_gpio_set_led(BSP_LED_STATUS, fault);
        break;

    case APP_STATE_SLEEP:
        bsp_gpio_set_led(BSP_LED_RUN, ((now_ms % 2000U) < 60U));
        bsp_gpio_set_led(BSP_LED_RECORD, false);
        bsp_gpio_set_led(BSP_LED_STATUS, false);
        break;

    case APP_STATE_BOOT:
        bsp_gpio_set_led(BSP_LED_RUN, false);
        bsp_gpio_set_led(BSP_LED_RECORD, false);
        bsp_gpio_set_led(BSP_LED_STATUS, fast);
        break;

    case APP_STATE_INIT:
    default:
        bsp_gpio_set_led(BSP_LED_RUN, false);
        bsp_gpio_set_led(BSP_LED_RECORD, false);
        bsp_gpio_set_led(BSP_LED_STATUS, false);
        break;
    }
}

const char *app_state_name(app_state_t state)
{
    switch(state) {
    case APP_STATE_INIT:
        return "INIT";
    case APP_STATE_MONITOR:
        return "MON";
    case APP_STATE_RECORD:
        return "REC";
    case APP_STATE_REPLAY:
        return "RPL";
    case APP_STATE_CONFIG:
        return "CFG";
    case APP_STATE_SLEEP:
        return "SLP";
    case APP_STATE_BOOT:
        return "BOT";
    default:
        return "UNK";
    }
}
