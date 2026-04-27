#ifndef BL_UART_H
#define BL_UART_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BL_CHAN_NONE = -1,
    BL_CHAN_DEBUG = 0,
    BL_CHAN_BT    = 1
} bl_channel_t;

void bl_uart_reset(void);
void bl_uart_poll(void);
bool bl_uart_frame_ready(void);
void bl_uart_process_frame(void);
uint32_t bl_uart_received_bytes(void);
bl_channel_t bl_uart_locked_channel(void);

#endif
