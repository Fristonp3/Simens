#include "app/app_protocol.h"

#include <string.h>

#include "app/app_state.h"
#include "app/app_storage.h"
#include "bootloader/bl_main.h"
#include "bsp/bsp_dac.h"
#include "bsp/bsp_rtc.h"
#include "service/srv_protocol.h"
#include "service/srv_power.h"
#include "service/srv_settings.h"
#include "service/srv_storage.h"

static bool s_bt_respond;  /* true = send response on BT channel */

static uint16_t app_protocol_read_u16_be(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static void app_protocol_write_u16_be(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)(value & 0xFFU);
}

static void app_protocol_pack_datetime(const rtc_datetime_t *datetime, uint8_t *payload)
{
    payload[0] = (uint8_t)(datetime->year % 100U);
    payload[1] = datetime->month;
    payload[2] = datetime->day;
    payload[3] = datetime->hour;
    payload[4] = datetime->minute;
    payload[5] = datetime->second;
}

static void app_protocol_send_response(uint8_t command, const uint8_t *payload, uint8_t length)
{
    if (s_bt_respond) {
        srv_protocol_send_frame_bt(command, payload, length);
    } else {
        srv_protocol_send_frame(command, payload, length);
    }
}

static void app_protocol_send_status_rsp(uint8_t command, app_result_t result)
{
    uint8_t payload[1];
    payload[0] = (uint8_t)result;
    app_protocol_send_response(command, payload, 1U);
}

static void app_protocol_send_sample(const app_context_t *ctx)
{
    uint8_t payload[9];

    app_protocol_write_u16_be(&payload[0], ctx->latest_sample.raw_adc);
    app_protocol_write_u16_be(&payload[2], ctx->latest_sample.voltage_mv);
    app_protocol_write_u16_be(&payload[4], (uint16_t)ctx->latest_sample.temperature_centi_c);
    app_protocol_write_u16_be(&payload[6], ctx->latest_sample.dac_raw);
    payload[8] = ctx->latest_sample.status;

    app_protocol_send_response(APP_CMD_READ_SAMPLE, payload, sizeof(payload));
}

static void app_protocol_send_status(const app_context_t *ctx)
{
    uint8_t payload[10];
    uint8_t flags = 0U;

    if (ctx->status.recording_enabled) { flags |= 0x01U; }
    if (ctx->status.storage_full)     { flags |= 0x02U; }
    if (ctx->status.sleep_armed)      { flags |= 0x04U; }
    if (ctx->status.uart_online)      { flags |= 0x08U; }
    if (ctx->status.boot_requested)   { flags |= 0x10U; }

    payload[0] = (uint8_t)ctx->status.current_state;
    payload[1] = flags;
    app_protocol_write_u16_be(&payload[2], ctx->status.record_count);
    app_protocol_write_u16_be(&payload[4], (uint16_t)ctx->config.sample_period_ms);
    app_protocol_write_u16_be(&payload[6], (uint16_t)ctx->config.record_period_ms);
    app_protocol_write_u16_be(&payload[8], ctx->config.dac_raw);

    app_protocol_send_response(APP_CMD_QUERY_STATUS, payload, sizeof(payload));
}

static void app_protocol_handle_read_history(app_context_t *ctx, const srv_protocol_frame_t *frame)
{
    sample_record_t records[PROJECT_HISTORY_BURST_MAX];
    uint8_t response[1U + PROJECT_HISTORY_BURST_MAX * 15U];
    uint16_t start, count;
    uint16_t offset = 0U, index;

    if (frame->length < 3U) {
        app_protocol_send_status_rsp(APP_CMD_READ_HISTORY, APP_RESULT_ARG);
        return;
    }

    start = app_protocol_read_u16_be(&frame->payload[0]);
    count = frame->payload[2];
    if (count == 0U) { count = 1U; }
    if (count > PROJECT_HISTORY_BURST_MAX) { count = PROJECT_HISTORY_BURST_MAX; }

    count = srv_storage_read_range(start, count, records);
    response[offset++] = (uint8_t)count;

    for (index = 0U; index < count; ++index) {
        response[offset++] = (uint8_t)(records[index].timestamp.year % 100U);
        response[offset++] = records[index].timestamp.month;
        response[offset++] = records[index].timestamp.day;
        response[offset++] = records[index].timestamp.hour;
        response[offset++] = records[index].timestamp.minute;
        response[offset++] = records[index].timestamp.second;
        response[offset++] = (uint8_t)(records[index].raw_adc >> 8U);
        response[offset++] = (uint8_t)(records[index].raw_adc & 0xFFU);
        response[offset++] = (uint8_t)(records[index].voltage_mv >> 8U);
        response[offset++] = (uint8_t)(records[index].voltage_mv & 0xFFU);
        response[offset++] = (uint8_t)(((uint16_t)records[index].temperature_centi_c) >> 8U);
        response[offset++] = (uint8_t)(((uint16_t)records[index].temperature_centi_c) & 0xFFU);
        response[offset++] = (uint8_t)(records[index].dac_raw >> 8U);
        response[offset++] = (uint8_t)(records[index].dac_raw & 0xFFU);
        response[offset++] = records[index].status;
    }

    app_protocol_send_response(APP_CMD_READ_HISTORY, response, (uint8_t)offset);
    ctx->display_dirty = true;
}

static void app_protocol_dispatch(app_context_t *ctx, srv_protocol_frame_t *frame, uint32_t now_ms)
{
    switch (frame->command) {
    case APP_CMD_READ_SAMPLE:
        app_protocol_send_sample(ctx);
        break;

    case APP_CMD_SET_DAC:
        if (frame->length != 2U) {
            app_protocol_send_status_rsp(APP_CMD_SET_DAC, APP_RESULT_ARG);
        } else {
            ctx->config.dac_raw = app_protocol_read_u16_be(frame->payload);
            bsp_dac_set_raw(ctx->config.dac_raw);
            srv_settings_save(&ctx->config);
            ctx->display_dirty = true;
            app_protocol_send_status_rsp(APP_CMD_SET_DAC, APP_RESULT_OK);
        }
        break;

    case APP_CMD_SET_TIME:
        if (frame->length != 6U) {
            app_protocol_send_status_rsp(APP_CMD_SET_TIME, APP_RESULT_ARG);
        } else {
            rtc_datetime_t time = {
                (uint16_t)(2000U + frame->payload[0]),
                frame->payload[1], frame->payload[2],
                frame->payload[3], frame->payload[4], frame->payload[5], 0U
            };
            bsp_rtc_set_datetime(&time);
            ctx->display_dirty = true;
            app_protocol_send_status_rsp(APP_CMD_SET_TIME, APP_RESULT_OK);
        }
        break;

    case APP_CMD_GET_TIME:
    {
        uint8_t payload[6];
        rtc_datetime_t time;
        bsp_rtc_get_datetime(&time);
        app_protocol_pack_datetime(&time, payload);
        app_protocol_send_response(APP_CMD_GET_TIME, payload, sizeof(payload));
        break;
    }

    case APP_CMD_START_RECORD:
        if (frame->length == 2U) {
            ctx->config.record_period_ms = app_protocol_read_u16_be(frame->payload);
            if (ctx->config.record_period_ms == 0U) {
                ctx->config.record_period_ms = PROJECT_DEFAULT_RECORD_PERIOD_MS;
            }
            srv_settings_save(&ctx->config);
        }
        (void)app_state_set(ctx, APP_STATE_RECORD, now_ms);
        app_protocol_send_status_rsp(APP_CMD_START_RECORD, APP_RESULT_OK);
        break;

    case APP_CMD_STOP_RECORD:
        (void)app_state_set(ctx, APP_STATE_MONITOR, now_ms);
        app_protocol_send_status_rsp(APP_CMD_STOP_RECORD, APP_RESULT_OK);
        break;

    case APP_CMD_READ_HISTORY:
        app_protocol_handle_read_history(ctx, frame);
        break;

    case APP_CMD_ERASE_HISTORY:
        app_storage_clear(ctx);
        app_protocol_send_status_rsp(APP_CMD_ERASE_HISTORY, APP_RESULT_OK);
        break;

    case APP_CMD_QUERY_STATUS:
        app_protocol_send_status(ctx);
        break;

    case APP_CMD_ENTER_BOOT:
        bl_main_request_enter();
        (void)app_state_set(ctx, APP_STATE_BOOT, now_ms);
        app_protocol_send_status_rsp(APP_CMD_ENTER_BOOT, APP_RESULT_OK);
        break;

    case APP_CMD_SWITCH_MODE:
        if ((frame->length != 1U) || (!app_state_set_by_mode_id(ctx, frame->payload[0], now_ms))) {
            app_protocol_send_status_rsp(APP_CMD_SWITCH_MODE, APP_RESULT_ARG);
        } else {
            app_protocol_send_status_rsp(APP_CMD_SWITCH_MODE, APP_RESULT_OK);
        }
        break;

    default:
        app_protocol_send_status_rsp(frame->command, APP_RESULT_ERROR);
        break;
    }
}

void app_protocol_poll(app_context_t *ctx, uint32_t now_ms)
{
    srv_protocol_frame_t frame;

    /* Poll UART0 (debug) first */
    while (srv_protocol_poll(&frame)) {
        s_bt_respond = false;
        srv_power_mark_activity(now_ms);
        ctx->status.last_activity_ms = now_ms;
        app_protocol_dispatch(ctx, &frame, now_ms);
    }

    /* Poll UART2 (BT) */
    while (srv_protocol_poll_bt(&frame)) {
        s_bt_respond = true;
        srv_power_mark_activity(now_ms);
        ctx->status.last_activity_ms = now_ms;
        app_protocol_dispatch(ctx, &frame, now_ms);
    }
}
