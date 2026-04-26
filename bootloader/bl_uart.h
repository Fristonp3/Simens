#ifndef BL_UART_H
#define BL_UART_H

#include <stdbool.h>
#include <stdint.h>

void bl_uart_reset(void);
void bl_uart_feed_byte(uint8_t byte);
bool bl_uart_frame_complete(void);
void bl_uart_process_frame(void);
uint32_t bl_uart_received_bytes(void);

#endif
