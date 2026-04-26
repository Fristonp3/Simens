#include "bl_uart.h"

#include <string.h>

#include "bl_flash.h"
#include "bl_jump.h"
#include "bsp/bsp_uart.h"
#include "common/crc16.h"

/* Bootloader frame format:
   Header:  0x55 0xAA
   Command: 1 byte
   Length:  1 byte (payload length)
   Payload: N bytes
   CRC16:   2 bytes (over command + length + payload)

   Commands:
   0xA0 - Erase application area
   0xA1 - Write firmware chunk (address:4B + data:N)
   0xA2 - Verify and jump (CRC16:2B over entire firmware)
*/

#define BL_CMD_ERASE     0xA0U
#define BL_CMD_WRITE     0xA1U
#define BL_CMD_VERIFY    0xA2U

#define BL_HEADER0       0x55U
#define BL_HEADER1       0xAAU
#define BL_MAX_PAYLOAD   128U

typedef enum {
    BL_STATE_HEADER0 = 0,
    BL_STATE_HEADER1,
    BL_STATE_COMMAND,
    BL_STATE_LENGTH,
    BL_STATE_PAYLOAD,
    BL_STATE_CRC_LOW,
    BL_STATE_CRC_HIGH
} bl_parser_state_t;

static bl_parser_state_t s_state = BL_STATE_HEADER0;
static uint8_t s_command;
static uint8_t s_length;
static uint8_t s_payload[BL_MAX_PAYLOAD];
static uint8_t s_offset;
static uint8_t s_crc_low;
static uint32_t s_received_bytes;
static bool s_frame_ready;

void bl_uart_reset(void)
{
    s_state = BL_STATE_HEADER0;
    s_received_bytes = 0U;
    s_frame_ready = false;
}

void bl_uart_feed_byte(uint8_t byte)
{
    s_received_bytes++;

    switch (s_state) {
    case BL_STATE_HEADER0:
        if (byte == BL_HEADER0) {
            s_state = BL_STATE_HEADER1;
        }
        break;

    case BL_STATE_HEADER1:
        if (byte == BL_HEADER1) {
            s_state = BL_STATE_COMMAND;
        } else if (byte != BL_HEADER0) {
            s_state = BL_STATE_HEADER0;
        }
        break;

    case BL_STATE_COMMAND:
        s_command = byte;
        s_state = BL_STATE_LENGTH;
        break;

    case BL_STATE_LENGTH:
        s_length = byte;
        s_offset = 0U;
        if (s_length > BL_MAX_PAYLOAD) {
            s_state = BL_STATE_HEADER0;
        } else if (s_length == 0U) {
            s_state = BL_STATE_CRC_LOW;
        } else {
            s_state = BL_STATE_PAYLOAD;
        }
        break;

    case BL_STATE_PAYLOAD:
        s_payload[s_offset++] = byte;
        if (s_offset >= s_length) {
            s_state = BL_STATE_CRC_LOW;
        }
        break;

    case BL_STATE_CRC_LOW:
        s_crc_low = byte;
        s_state = BL_STATE_CRC_HIGH;
        break;

    case BL_STATE_CRC_HIGH:
    {
        uint8_t crc_buf[2U + BL_MAX_PAYLOAD];
        uint16_t crc;

        crc_buf[0] = s_command;
        crc_buf[1] = s_length;
        if (s_length > 0U) {
            memcpy(&crc_buf[2], s_payload, s_length);
        }
        crc = crc16_modbus(crc_buf, (size_t)(s_length + 2U));

        if ((s_crc_low == (uint8_t)(crc & 0xFFU)) &&
            (byte == (uint8_t)((crc >> 8U) & 0xFFU))) {
            s_frame_ready = true;
        }

        s_state = BL_STATE_HEADER0;
        break;
    }

    default:
        s_state = BL_STATE_HEADER0;
        break;
    }
}

bool bl_uart_frame_complete(void)
{
    return s_frame_ready;
}

static void bl_send_response(uint8_t command, uint8_t result)
{
    uint8_t response[2 + 2 + 1 + 2];
    uint16_t crc;
    uint8_t idx = 0U;

    response[idx++] = BL_HEADER0;
    response[idx++] = BL_HEADER1;
    response[idx++] = command;
    response[idx++] = 1U;
    response[idx++] = result;

    crc = crc16_modbus(&response[2], 3U);
    response[idx++] = (uint8_t)(crc & 0xFFU);
    response[idx++] = (uint8_t)((crc >> 8U) & 0xFFU);

    bsp_uart_send_bytes(response, idx);
}

void bl_uart_process_frame(void)
{
    s_frame_ready = false;

    switch (s_command) {
    case BL_CMD_ERASE:
        bl_flash_erase_app();
        bl_send_response(BL_CMD_ERASE, 0x00U);
        break;

    case BL_CMD_WRITE:
        if (s_length >= 4U) {
            uint32_t addr = ((uint32_t)s_payload[0] << 24U) |
                            ((uint32_t)s_payload[1] << 16U) |
                            ((uint32_t)s_payload[2] << 8U) |
                            ((uint32_t)s_payload[3]);
            uint8_t data_len = (uint8_t)(s_length - 4U);
            if (bl_flash_write_chunk(addr, &s_payload[4], data_len)) {
                bl_send_response(BL_CMD_WRITE, 0x00U);
            } else {
                bl_send_response(BL_CMD_WRITE, 0x01U);
            }
        } else {
            bl_send_response(BL_CMD_WRITE, 0x02U);
        }
        break;

    case BL_CMD_VERIFY:
        if (s_length == 2U) {
            uint16_t expected_crc = ((uint16_t)s_payload[0] << 8U) | s_payload[1];
            if (bl_flash_verify_crc(expected_crc)) {
                bl_flash_mark_complete();
                bl_send_response(BL_CMD_VERIFY, 0x00U);
            } else {
                bl_send_response(BL_CMD_VERIFY, 0x01U);
            }
        } else {
            bl_send_response(BL_CMD_VERIFY, 0x02U);
        }
        break;

    default:
        bl_send_response(s_command, 0xFFU);
        break;
    }
}

uint32_t bl_uart_received_bytes(void)
{
    return s_received_bytes;
}
