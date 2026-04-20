#ifndef BL_UART_H
#define BL_UART_H

#include <stdint.h>

void bl_uart_reset(void);
void bl_uart_feed(const uint8_t *data, uint16_t length);
uint32_t bl_uart_received_bytes(void);

#endif
