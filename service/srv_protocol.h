#ifndef SRV_PROTOCOL_H
#define SRV_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"

typedef struct {
    uint8_t command;
    uint8_t length;
    uint8_t payload[PROJECT_PROTOCOL_MAX_PAYLOAD];
} srv_protocol_frame_t;

void srv_protocol_init(void);
bool srv_protocol_poll(srv_protocol_frame_t *frame);
bool srv_protocol_poll_bt(srv_protocol_frame_t *frame);
void srv_protocol_send_frame(uint8_t command, const uint8_t *payload, uint8_t length);
void srv_protocol_send_frame_bt(uint8_t command, const uint8_t *payload, uint8_t length);
void srv_protocol_send_status(uint8_t command, app_result_t result);

#endif
