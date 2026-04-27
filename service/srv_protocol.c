#include "service/srv_protocol.h"

#include <string.h>

#include "bsp/bsp_uart.h"
#include "common/crc16.h"

typedef enum {
    PROTO_WAIT_HEADER0 = 0,
    PROTO_WAIT_HEADER1,
    PROTO_WAIT_COMMAND,
    PROTO_WAIT_LENGTH,
    PROTO_WAIT_PAYLOAD,
    PROTO_WAIT_CRC_LOW,
    PROTO_WAIT_CRC_HIGH
} protocol_parser_state_t;

typedef struct {
    protocol_parser_state_t state;
    uint8_t command;
    uint8_t length;
    uint8_t payload[PROJECT_PROTOCOL_MAX_PAYLOAD];
    uint8_t offset;
    uint8_t crc_low;
} protocol_parser_t;

static protocol_parser_t s_parser;
static protocol_parser_t s_parser_bt;

static void protocol_parser_reset(protocol_parser_t *p)
{
    memset(p, 0, sizeof(*p));
    p->state = PROTO_WAIT_HEADER0;
}

static bool protocol_parser_feed(protocol_parser_t *p, uint8_t byte, srv_protocol_frame_t *frame)
{
    switch (p->state) {
    case PROTO_WAIT_HEADER0:
        if (byte == PROJECT_PROTOCOL_HEADER0) {
            p->state = PROTO_WAIT_HEADER1;
        }
        break;

    case PROTO_WAIT_HEADER1:
        if (byte == PROJECT_PROTOCOL_HEADER1) {
            p->state = PROTO_WAIT_COMMAND;
        } else if (byte != PROJECT_PROTOCOL_HEADER0) {
            p->state = PROTO_WAIT_HEADER0;
        }
        break;

    case PROTO_WAIT_COMMAND:
        p->command = byte;
        p->state = PROTO_WAIT_LENGTH;
        break;

    case PROTO_WAIT_LENGTH:
        p->length = byte;
        p->offset = 0U;
        if (p->length > PROJECT_PROTOCOL_MAX_PAYLOAD) {
            protocol_parser_reset(p);
        } else if (p->length == 0U) {
            p->state = PROTO_WAIT_CRC_LOW;
        } else {
            p->state = PROTO_WAIT_PAYLOAD;
        }
        break;

    case PROTO_WAIT_PAYLOAD:
        p->payload[p->offset++] = byte;
        if (p->offset >= p->length) {
            p->state = PROTO_WAIT_CRC_LOW;
        }
        break;

    case PROTO_WAIT_CRC_LOW:
        p->crc_low = byte;
        p->state = PROTO_WAIT_CRC_HIGH;
        break;

    case PROTO_WAIT_CRC_HIGH:
    {
        uint8_t crc_input[2U + PROJECT_PROTOCOL_MAX_PAYLOAD];
        uint16_t crc;

        crc_input[0] = p->command;
        crc_input[1] = p->length;
        if (p->length > 0U) {
            memcpy(&crc_input[2], p->payload, p->length);
        }
        crc = crc16_modbus(crc_input, (size_t)(p->length + 2U));
        if ((p->crc_low == (uint8_t)(crc & 0xFFU)) &&
            (byte == (uint8_t)((crc >> 8U) & 0xFFU))) {
            frame->command = p->command;
            frame->length = p->length;
            if (p->length > 0U) {
                memcpy(frame->payload, p->payload, p->length);
            }
            protocol_parser_reset(p);
            return true;
        }
        protocol_parser_reset(p);
        break;
    }

    default:
        protocol_parser_reset(p);
        break;
    }

    return false;
}

void srv_protocol_init(void)
{
    protocol_parser_reset(&s_parser);
    protocol_parser_reset(&s_parser_bt);
}

bool srv_protocol_poll(srv_protocol_frame_t *frame)
{
    uint8_t byte;
    while (bsp_uart_read_byte(&byte)) {
        if (protocol_parser_feed(&s_parser, byte, frame)) {
            return true;
        }
    }
    return false;
}

bool srv_protocol_poll_bt(srv_protocol_frame_t *frame)
{
    uint8_t byte;
    while (bsp_uart_bt_read_byte(&byte)) {
        if (protocol_parser_feed(&s_parser_bt, byte, frame)) {
            return true;
        }
    }
    return false;
}

static void srv_protocol_build_frame(uint8_t command, const uint8_t *payload, uint8_t length,
                                      uint8_t *frame_out, uint16_t *frame_len)
{
    uint16_t crc;
    uint16_t index = 0U;

    frame_out[index++] = PROJECT_PROTOCOL_HEADER0;
    frame_out[index++] = PROJECT_PROTOCOL_HEADER1;
    frame_out[index++] = command;
    frame_out[index++] = length;

    if ((length > 0U) && (payload != 0)) {
        memcpy(&frame_out[index], payload, length);
        index = (uint16_t)(index + length);
    }

    crc = crc16_modbus(&frame_out[2], (size_t)(length + 2U));
    frame_out[index++] = (uint8_t)(crc & 0xFFU);
    frame_out[index++] = (uint8_t)((crc >> 8U) & 0xFFU);

    *frame_len = index;
}

void srv_protocol_send_frame(uint8_t command, const uint8_t *payload, uint8_t length)
{
    uint8_t frame[2U + 2U + PROJECT_PROTOCOL_MAX_PAYLOAD + 2U];
    uint16_t frame_len;

    srv_protocol_build_frame(command, payload, length, frame, &frame_len);
    bsp_uart_send_bytes(frame, frame_len);
}

void srv_protocol_send_frame_bt(uint8_t command, const uint8_t *payload, uint8_t length)
{
    uint8_t frame[2U + 2U + PROJECT_PROTOCOL_MAX_PAYLOAD + 2U];
    uint16_t frame_len;

    srv_protocol_build_frame(command, payload, length, frame, &frame_len);
    bsp_uart_bt_send_bytes(frame, frame_len);
}

void srv_protocol_send_status(uint8_t command, app_result_t result)
{
    uint8_t payload[1];
    payload[0] = (uint8_t)result;
    srv_protocol_send_frame(command, payload, 1U);
}
