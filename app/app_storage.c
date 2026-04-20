#include "app/app_storage.h"

#include "service/srv_storage.h"

bool app_storage_append_latest(app_context_t *ctx)
{
    sample_record_t record = ctx->latest_sample;

    if(ctx->status.storage_full) {
        record.status |= SAMPLE_STATUS_STORAGE_FULL;
    }

    if(!srv_storage_append(&record)) {
        ctx->status.storage_full = true;
        ctx->status.last_error = APP_RESULT_BUSY;
        return false;
    }

    ctx->status.record_count = srv_storage_count();
    ctx->status.storage_full = srv_storage_is_full();
    if(ctx->status.record_count > 0U) {
        ctx->replay_index = (uint16_t)(ctx->status.record_count - 1U);
    }

    ctx->display_dirty = true;
    return true;
}

bool app_storage_get_replay_record(const app_context_t *ctx, sample_record_t *record)
{
    if(ctx->status.record_count == 0U) {
        return false;
    }

    return srv_storage_read(ctx->replay_index, record);
}

void app_storage_next_record(app_context_t *ctx)
{
    if(ctx->status.record_count == 0U) {
        return;
    }

    ctx->replay_index = (uint16_t)((ctx->replay_index + 1U) % ctx->status.record_count);
    ctx->display_dirty = true;
}

void app_storage_previous_record(app_context_t *ctx)
{
    if(ctx->status.record_count == 0U) {
        return;
    }

    if(ctx->replay_index == 0U) {
        ctx->replay_index = (uint16_t)(ctx->status.record_count - 1U);
    } else {
        --ctx->replay_index;
    }

    ctx->display_dirty = true;
}

void app_storage_clear(app_context_t *ctx)
{
    srv_storage_clear();
    ctx->status.record_count = 0U;
    ctx->status.storage_full = false;
    ctx->replay_index = 0U;
    ctx->display_dirty = true;
}
