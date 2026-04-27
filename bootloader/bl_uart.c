#include "bl_uart.h"

#include <string.h>

#include "bl_flash.h"
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

typedef struct {
    bl_parser_state_t state;
    uint8_t command;
    uint8_t length;
    uint8_t payload[BL_MAX_PAYLOAD];
    uint8_t offset;
    uint8_t crc_low;
    uint32_t byte_count;
    bool frame_ready;
} bl_channel_parser_t;

static bl_channel_parser_t s_parsers[2];
static bl_channel_t s_locked_channel = BL_CHAN_NONE;
static bool s_frame_ready;

static void bl_parser_reset(bl_channel_parser_t *p)
{
    memset(p, 0, sizeof(*p));
    p->state = BL_STATE_HEADER0;
}

static bool bl_parser_feed(bl_channel_parser_t *p, uint8_t byte)
{
    p->byte_count++;

    switch (p->state) {
    case BL_STATE_HEADER0:
        if (byte == BL_HEADER0) {
            p->state = BL_STATE_HEADER1;
        }
        break;

    case BL_STATE_HEADER1:
        if (byte == BL_HEADER1) {
            p->state = BL_STATE_COMMAND;
        } else if (byte != BL_HEADER0) {
            p->state = BL_STATE_HEADER0;
        }
        break;

    case BL_STATE_COMMAND:
        p->command = byte;
        p->state = BL_STATE_LENGTH;
        break;

    case BL_STATE_LENGTH:
        p->length = byte;
        p->offset = 0U;
        if (p->length > BL_MAX_PAYLOAD) {
            p->state = BL_STATE_HEADER0;
        } else if (p->length == 0U) {
            p->state = BL_STATE_CRC_LOW;
        } else {
            p->state = BL_STATE_PAYLOAD;
        }
        break;

    case BL_STATE_PAYLOAD:
        p->payload[p->offset++] = byte;
        if (p->offset >= p->length) {
            p->state = BL_STATE_CRC_LOW;
        }
        break;

    case BL_STATE_CRC_LOW:
        p->crc_low = byte;
        p->state = BL_STATE_CRC_HIGH;
        break;

    case BL_STATE_CRC_HIGH:
    {
        uint8_t crc_buf[2U + BL_MAX_PAYLOAD];
        uint16_t crc;

        crc_buf[0] = p->command;
        crc_buf[1] = p->length;
        if (p->length > 0U) {
            memcpy(&crc_buf[2], p->payload, p->length);
        }
        crc = crc16_modbus(crc_buf, (size_t)(p->length + 2U));

        p->state = BL_STATE_HEADER0;

        if ((p->crc_low == (uint8_t)(crc & 0xFFU)) &&
            (byte == (uint8_t)((crc >> 8U) & 0xFFU))) {
            p->frame_ready = true;
            return true;
        }
        break;
    }

    default:
        p->state = BL_STATE_HEADER0;
        break;
    }

    return false;
}

static void bl_send_response(bl_channel_t chan, uint8_t command, uint8_t result)
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

    if (chan == BL_CHAN_BT) {
        bsp_uart_bt_send_bytes(response, idx);
    } else {
        bsp_uart_send_bytes(response, idx);
    }
}

static void bl_poll_channel(bl_channel_t chan)
{
    bool (*read_fn)(uint8_t *);
    bl_channel_parser_t *parser;

    if (s_locked_channel != BL_CHAN_NONE && s_locked_channel != chan) {
        /* Drain bytes from non-locked channel */
        uint8_t dummy;
        if (chan == BL_CHAN_BT) {
            while (bsp_uart_bt_read_byte(&dummy)) {}
        } else {
            while (bsp_uart_read_byte(&dummy)) {}
        }
        return;
    }

    read_fn = (chan == BL_CHAN_BT) ? bsp_uart_bt_read_byte : bsp_uart_read_byte;
    parser = &s_parsers[chan];

    uint8_t byte;
    while (read_fn(&byte)) {
        if (bl_parser_feed(parser, byte)) {
            if (s_locked_channel == BL_CHAN_NONE) {
                s_locked_channel = chan;
            }
            s_frame_ready = true;
            return;
        }
    }
}

void bl_uart_reset(void)
{
    bl_parser_reset(&s_parsers[0]);
    bl_parser_reset(&s_parsers[1]);
    s_locked_channel = BL_CHAN_NONE;
    s_frame_ready = false;
}

void bl_uart_poll(void)
{
    if (s_frame_ready) { return; }

    bl_poll_channel(BL_CHAN_DEBUG);
    if (!s_frame_ready) {
        bl_poll_channel(BL_CHAN_BT);
    }
}

bool bl_uart_frame_ready(void)
{
    return s_frame_ready;
}

void bl_uart_process_frame(void)
{
    bl_channel_parser_t *parser = &s_parsers[s_locked_channel];

    s_frame_ready = false;
    parser->frame_ready = false;

    switch (parser->command) {
    case BL_CMD_ERASE:
        bl_flash_erase_app();
        bl_send_response(s_locked_channel, BL_CMD_ERASE, 0x00U);
        break;

    case BL_CMD_WRITE:
        if (parser->length >= 4U) {
            uint32_t addr = ((uint32_t)parser->payload[0] << 24U) |
                            ((uint32_t)parser->payload[1] << 16U) |
                            ((uint32_t)parser->payload[2] << 8U) |
                            ((uint32_t)parser->payload[3]);
            uint8_t data_len = (uint8_t)(parser->length - 4U);
            if (bl_flash_write_chunk(addr, &parser->payload[4], data_len)) {
                bl_send_response(s_locked_channel, BL_CMD_WRITE, 0x00U);
            } else {
                bl_send_response(s_locked_channel, BL_CMD_WRITE, 0x01U);
            }
        } else {
            bl_send_response(s_locked_channel, BL_CMD_WRITE, 0x02U);
        }
        break;

    case BL_CMD_VERIFY:
        if (parser->length == 2U) {
            uint16_t expected_crc = ((uint16_t)parser->payload[0] << 8U) | parser->payload[1];
            if (bl_flash_verify_crc(expected_crc)) {
                bl_flash_mark_complete();
                bl_send_response(s_locked_channel, BL_CMD_VERIFY, 0x00U);
            } else {
                bl_send_response(s_locked_channel, BL_CMD_VERIFY, 0x01U);
            }
        } else {
            bl_send_response(s_locked_channel, BL_CMD_VERIFY, 0x02U);
        }
        break;

    default:
        bl_send_response(s_locked_channel, parser->command, 0xFFU);
        break;
    }
}

uint32_t bl_uart_received_bytes(void)
{
    return s_parsers[0].byte_count + s_parsers[1].byte_count;
}

bl_channel_t bl_uart_locked_channel(void)
{
    return s_locked_channel;
}
