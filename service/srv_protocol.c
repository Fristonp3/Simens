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

static void protocol_parser_reset(void)
{
    memset(&s_parser, 0, sizeof(s_parser));
    s_parser.state = PROTO_WAIT_HEADER0;
}

void srv_protocol_init(void)
{
    protocol_parser_reset();
}

bool srv_protocol_poll(srv_protocol_frame_t *frame)
{
    uint8_t byte;

    while(bsp_uart_read_byte(&byte)) {
        switch(s_parser.state) {
        case PROTO_WAIT_HEADER0:
            if(byte == PROJECT_PROTOCOL_HEADER0) {
                s_parser.state = PROTO_WAIT_HEADER1;
            }
            break;

        case PROTO_WAIT_HEADER1:
            if(byte == PROJECT_PROTOCOL_HEADER1) {
                s_parser.state = PROTO_WAIT_COMMAND;
            } else if(byte != PROJECT_PROTOCOL_HEADER0) {
                s_parser.state = PROTO_WAIT_HEADER0;
            }
            break;

        case PROTO_WAIT_COMMAND:
            s_parser.command = byte;
            s_parser.state = PROTO_WAIT_LENGTH;
            break;

        case PROTO_WAIT_LENGTH:
            s_parser.length = byte;
            s_parser.offset = 0U;
            if(s_parser.length > PROJECT_PROTOCOL_MAX_PAYLOAD) {
                protocol_parser_reset();
            } else if(s_parser.length == 0U) {
                s_parser.state = PROTO_WAIT_CRC_LOW;
            } else {
                s_parser.state = PROTO_WAIT_PAYLOAD;
            }
            break;

        case PROTO_WAIT_PAYLOAD:
            s_parser.payload[s_parser.offset++] = byte;
            if(s_parser.offset >= s_parser.length) {
                s_parser.state = PROTO_WAIT_CRC_LOW;
            }
            break;

        case PROTO_WAIT_CRC_LOW:
            s_parser.crc_low = byte;
            s_parser.state = PROTO_WAIT_CRC_HIGH;
            break;

        case PROTO_WAIT_CRC_HIGH:
        {
            uint8_t crc_input[2U + PROJECT_PROTOCOL_MAX_PAYLOAD];
            uint16_t crc;

            crc_input[0] = s_parser.command;
            crc_input[1] = s_parser.length;
            if(s_parser.length > 0U) {
                memcpy(&crc_input[2], s_parser.payload, s_parser.length);
            }
            crc = crc16_modbus(crc_input, (size_t)(s_parser.length + 2U));
            if((s_parser.crc_low == (uint8_t)(crc & 0xFFU)) &&
               (byte == (uint8_t)((crc >> 8U) & 0xFFU))) {
                frame->command = s_parser.command;
                frame->length = s_parser.length;
                if(s_parser.length > 0U) {
                    memcpy(frame->payload, s_parser.payload, s_parser.length);
                }
                protocol_parser_reset();
                return true;
            }
            protocol_parser_reset();
            break;
        }

        default:
            protocol_parser_reset();
            break;
        }
    }

    return false;
}

void srv_protocol_send_frame(uint8_t command, const uint8_t *payload, uint8_t length)
{
    uint8_t frame[2U + 2U + PROJECT_PROTOCOL_MAX_PAYLOAD + 2U];
    uint16_t crc;
    uint16_t index = 0U;

    frame[index++] = PROJECT_PROTOCOL_HEADER0;
    frame[index++] = PROJECT_PROTOCOL_HEADER1;
    frame[index++] = command;
    frame[index++] = length;

    if((length > 0U) && (payload != 0)) {
        memcpy(&frame[index], payload, length);
        index = (uint16_t)(index + length);
    }

    crc = crc16_modbus(&frame[2], (size_t)(length + 2U));
    frame[index++] = (uint8_t)(crc & 0xFFU);
    frame[index++] = (uint8_t)((crc >> 8U) & 0xFFU);

    bsp_uart_send_bytes(frame, index);
}

void srv_protocol_send_status(uint8_t command, app_result_t result)
{
    uint8_t payload[1];

    payload[0] = (uint8_t)result;
    srv_protocol_send_frame(command, payload, 1U);
}
