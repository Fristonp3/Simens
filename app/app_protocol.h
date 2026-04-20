#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include <stdint.h>

#include "app/app_main.h"

void app_protocol_poll(app_context_t *ctx, uint32_t now_ms);

#endif
