#include "bl_uart.h"

static uint32_t s_received_bytes;

void bl_uart_reset(void)
{
    s_received_bytes = 0U;
}

void bl_uart_feed(const uint8_t *data, uint16_t length)
{
    (void)data;
    s_received_bytes += length;
}

uint32_t bl_uart_received_bytes(void)
{
    return s_received_bytes;
}
