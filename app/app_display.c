#include "app/app_display.h"

#include <stdio.h>

#include "app/app_state.h"
#include "app/app_storage.h"
#include "service/srv_display.h"
#include "service/srv_storage.h"
#include "systick.h"

static void app_display_format_time(const rtc_datetime_t *time, char *buffer, size_t size)
{
    snprintf(buffer, size, "%02u:%02u:%02u", time->hour, time->minute, time->second);
}

static void app_display_format_temp(int16_t centi_c, char *buffer, size_t size)
{
    int16_t abs_value = centi_c;
    char sign = ' ';

    if(abs_value < 0) {
        sign = '-';
        abs_value = (int16_t)(-abs_value);
    }

    snprintf(buffer, size, "%c%3d.%02dC", sign, abs_value / 100, abs_value % 100);
}

static const char *app_display_status_text(uint8_t status)
{
    if((status & SAMPLE_STATUS_SENSOR_OPEN) != 0U) {
        return "OPEN";
    }
    if((status & SAMPLE_STATUS_SENSOR_SHORT) != 0U) {
        return "SHRT";
    }
    if((status & SAMPLE_STATUS_STORAGE_FULL) != 0U) {
        return "FULL";
    }

    return "NORM";
}

void app_display_render(app_context_t *ctx)
{
    char line0[40];
    char line1[40];
    char line2[40];
    char line3[40];
    char time_buffer[16];
    char temp_buffer[16];

    app_display_format_time(&ctx->latest_sample.timestamp, time_buffer, sizeof(time_buffer));
    app_display_format_temp(ctx->latest_sample.temperature_centi_c, temp_buffer, sizeof(temp_buffer));

    switch(ctx->status.current_state) {
    case APP_STATE_REPLAY:
    {
        sample_record_t record;
        if(app_storage_get_replay_record(ctx, &record)) {
            app_display_format_time(&record.timestamp, time_buffer, sizeof(time_buffer));
            app_display_format_temp(record.temperature_centi_c, temp_buffer, sizeof(temp_buffer));
            snprintf(line0, sizeof(line0), "RPL %u/%u %s",
                     ctx->replay_index + 1U, ctx->status.record_count, time_buffer);
            snprintf(line1, sizeof(line1), "ADC:%4u V:%4umV", record.raw_adc, record.voltage_mv);
            snprintf(line2, sizeof(line2), "TMP:%s", temp_buffer);
            snprintf(line3, sizeof(line3), "ST:%s", app_display_status_text(record.status));
        } else {
            snprintf(line0, sizeof(line0), "RPL EMPTY");
            snprintf(line1, sizeof(line1), "No history records");
            snprintf(line2, sizeof(line2), "NEXT/WAKE browse");
            snprintf(line3, sizeof(line3), "MODE switch state");
        }
        break;
    }

    case APP_STATE_CONFIG:
    {
        char select0 = (ctx->config_cursor == 0U) ? '>' : ' ';
        char select1 = (ctx->config_cursor == 1U) ? '>' : ' ';
        char select2 = (ctx->config_cursor == 2U) ? '>' : ' ';
        char select3 = (ctx->config_cursor == 3U) ? '>' : ' ';

        snprintf(line0, sizeof(line0), "%c SMP:%4lums", select0, (unsigned long)ctx->config.sample_period_ms);
        snprintf(line1, sizeof(line1), "%c DSP:%4lums", select1, (unsigned long)ctx->config.display_period_ms);
        snprintf(line2, sizeof(line2), "%c REC:%4lums", select2, (unsigned long)ctx->config.record_period_ms);
        snprintf(line3, sizeof(line3), "%c DAC:%4u Pg:%u", select3, ctx->config.dac_raw, ctx->config.display_page);
        break;
    }

    case APP_STATE_SLEEP:
        snprintf(line0, sizeof(line0), "SLEEP %s", time_buffer);
        snprintf(line1, sizeof(line1), "Wake by key/uart");
        snprintf(line2, sizeof(line2), "Prev:%s", app_state_name(ctx->status.previous_state));
        snprintf(line3, sizeof(line3), "Idle:%lus",
                 (unsigned long)((systick_ms_get() - ctx->status.last_activity_ms) / 1000U));
        break;

    case APP_STATE_BOOT:
        snprintf(line0, sizeof(line0), "BOOTLOADER READY");
        snprintf(line1, sizeof(line1), "UART upgrade window");
        snprintf(line2, sizeof(line2), "CMD 0x0A accepted");
        snprintf(line3, sizeof(line3), "Records:%u", ctx->status.record_count);
        break;

    case APP_STATE_INIT:
    case APP_STATE_MONITOR:
    case APP_STATE_RECORD:
    default:
        if(ctx->config.display_page == APP_PAGE_MAIN) {
            snprintf(line0, sizeof(line0), "%s %s %02u/%02u",
                     app_state_name(ctx->status.current_state), time_buffer,
                     ctx->latest_sample.timestamp.month, ctx->latest_sample.timestamp.day);
            snprintf(line1, sizeof(line1), "ADC:%4u V:%4umV", ctx->latest_sample.raw_adc, ctx->latest_sample.voltage_mv);
            snprintf(line2, sizeof(line2), "TMP:%s DAC:%4u", temp_buffer, ctx->config.dac_raw);
            snprintf(line3, sizeof(line3), "REC:%u/%u %s",
                     ctx->status.record_count, srv_storage_capacity(),
                     app_display_status_text(ctx->latest_sample.status));
        } else {
            snprintf(line0, sizeof(line0), "Pg1 %s", app_state_name(ctx->status.current_state));
            snprintf(line1, sizeof(line1), "UART:%s Boot:%u",
                     ctx->status.uart_online ? "ON" : "OFF",
                     ctx->status.boot_requested ? 1U : 0U);
            snprintf(line2, sizeof(line2), "Smp:%lums Rec:%lums",
                     (unsigned long)ctx->config.sample_period_ms,
                     (unsigned long)ctx->config.record_period_ms);
            snprintf(line3, sizeof(line3), "DAC:%umV Fault:%s",
                     (unsigned int)((ctx->config.dac_raw * 3300U) / 4095U),
                     app_display_status_text(ctx->latest_sample.status));
        }
        break;
    }

    srv_display_present(line0, line1, line2, line3);
}
