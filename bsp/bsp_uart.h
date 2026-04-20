#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

void bsp_uart_init(void);
void bsp_uart_irq_handler(void);
bool bsp_uart_read_byte(uint8_t *byte);
uint16_t bsp_uart_available(void);
void bsp_uart_send_bytes(const uint8_t *data, uint16_t length);
void bsp_uart_send_string(const char *text);
bool bsp_uart_take_activity_flag(void);

#endif
