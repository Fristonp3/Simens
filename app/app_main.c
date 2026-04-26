#include "app/app_main.h"

#include <string.h>

#include "app/app_display.h"
#include "app/app_protocol.h"
#include "app/app_state.h"
#include "app/app_storage.h"
#include "bootloader/bl_main.h"
#include "bsp/bsp_adc.h"
#include "bsp/bsp_dac.h"
#include "bsp/bsp_gpio.h"
#include "bsp/bsp_power.h"
#include "bsp/bsp_rtc.h"
#include "bsp/bsp_uart.h"
#include "service/srv_display.h"
#include "service/srv_power.h"
#include "service/srv_protocol.h"
#include "service/srv_sample.h"
#include "service/srv_settings.h"
#include "service/srv_storage.h"
#include "systick.h"

static app_context_t s_app;

static void app_main_update_runtime_flags(app_context_t *ctx)
{
    ctx->status.record_count = srv_storage_count();
    ctx->status.storage_full = srv_storage_is_full();
    ctx->status.uart_online = ((ctx->last_uart_ms != UINT32_MAX) &&
                               ((systick_ms_get() - ctx->last_uart_ms) <= 5000U));

    ctx->latest_sample.status &= (uint8_t)~(SAMPLE_STATUS_STORAGE_FULL | SAMPLE_STATUS_UART_ACTIVE);
    if(ctx->status.storage_full) {
        ctx->latest_sample.status |= SAMPLE_STATUS_STORAGE_FULL;
    }
    if(ctx->status.uart_online) {
        ctx->latest_sample.status |= SAMPLE_STATUS_UART_ACTIVE;
    }
}

void app_main_init(void)
{
    uint32_t now_ms = systick_ms_get();

    memset(&s_app, 0, sizeof(s_app));

    bsp_gpio_init();
    bsp_uart_init();
    bsp_rtc_init();
    bsp_adc_init();
    bsp_power_init();

    srv_display_init();
    srv_storage_init();
    srv_protocol_init();
    srv_sample_init();
    srv_power_init(now_ms);
    bl_main_init();

    s_app.config.sample_period_ms = PROJECT_DEFAULT_SAMPLE_PERIOD_MS;
    s_app.config.display_period_ms = PROJECT_DEFAULT_DISPLAY_PERIOD_MS;
    s_app.config.record_period_ms = PROJECT_DEFAULT_RECORD_PERIOD_MS;
    s_app.config.dac_raw = 2048U;
    s_app.config.display_page = APP_PAGE_MAIN;
    (void)srv_settings_load(&s_app.config);

    bsp_dac_init();
    bsp_dac_set_raw(s_app.config.dac_raw);

    s_app.last_uart_ms = UINT32_MAX;
    s_app.display_dirty = true;

    soft_timer_start_periodic(&s_app.sample_timer, now_ms, s_app.config.sample_period_ms);
    soft_timer_start_periodic(&s_app.display_timer, now_ms, s_app.config.display_period_ms);
    soft_timer_stop(&s_app.record_timer);

    app_state_init(&s_app, now_ms);
    s_app.latest_sample = *srv_sample_capture();
    app_main_update_runtime_flags(&s_app);
    app_display_render(&s_app);
    s_app.display_dirty = false;

    bsp_uart_send_string("\r\nIndustrial acquisition firmware ready.\r\n");
}

void app_main_process(void)
{
    uint32_t now_ms = systick_ms_get();
    bool wake_flag = false;

    bsp_gpio_poll(now_ms);

    if(bsp_uart_take_activity_flag()) {
        s_app.last_uart_ms = now_ms;
        srv_power_mark_activity(now_ms);
        s_app.status.last_activity_ms = now_ms;
    }

    wake_flag = bsp_gpio_take_wakeup_flag();
    if(wake_flag) {
        srv_power_wake(now_ms);
        s_app.status.last_activity_ms = now_ms;
        if(s_app.status.current_state == APP_STATE_SLEEP) {
            (void)app_state_set(&s_app, s_app.status.previous_state, now_ms);
        }
    }

    app_protocol_poll(&s_app, now_ms);
    app_state_process_keys(&s_app, now_ms);

    if((s_app.status.current_state != APP_STATE_SLEEP) &&
       (s_app.status.current_state != APP_STATE_BOOT) &&
       soft_timer_expired(&s_app.sample_timer, now_ms)) {
        s_app.latest_sample = *srv_sample_capture();
    }

    app_main_update_runtime_flags(&s_app);

    if((s_app.status.current_state == APP_STATE_RECORD) &&
       soft_timer_expired(&s_app.record_timer, now_ms)) {
        if(!app_storage_append_latest(&s_app)) {
            s_app.latest_sample.status |= SAMPLE_STATUS_STORAGE_FULL;
            (void)app_state_set(&s_app, APP_STATE_MONITOR, now_ms);
        }
    }

    if((s_app.status.current_state != APP_STATE_SLEEP) &&
       srv_power_should_enter_sleep(now_ms, s_app.status.current_state)) {
        (void)app_state_set(&s_app, APP_STATE_SLEEP, now_ms);
    }

    if(s_app.status.current_state == APP_STATE_BOOT) {
        bl_main_process();
        if(bl_main_should_jump()) {
            bl_main_do_jump();
            /* Never returns */
        }
        return;
    }

    app_state_update_leds(&s_app, now_ms);

    if(s_app.display_dirty || soft_timer_expired(&s_app.display_timer, now_ms)) {
        app_display_render(&s_app);
        s_app.display_dirty = false;
    }

    if(s_app.status.current_state == APP_STATE_SLEEP) {
        bsp_power_enter_sleep();
    }
}

void app_main_run(void)
{
    while(1) {
        app_main_process();
    }
}
