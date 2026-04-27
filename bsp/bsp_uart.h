#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

/* UART channel selector */
typedef enum {
    BSP_UART_DEBUG = 0,
    BSP_UART_BT    = 1
} bsp_uart_channel_t;

void bsp_uart_init(void);
void bsp_uart_irq_handler(void);
bool bsp_uart_read_byte(uint8_t *byte);
uint16_t bsp_uart_available(void);
void bsp_uart_send_bytes(const uint8_t *data, uint16_t length);
void bsp_uart_send_string(const char *text);
bool bsp_uart_take_activity_flag(void);

/* BT channel (UART2 / HC-06) */
void bsp_uart_bt_init(void);
void bsp_uart_bt_irq_handler(void);
bool bsp_uart_bt_read_byte(uint8_t *byte);
uint16_t bsp_uart_bt_available(void);
void bsp_uart_bt_send_bytes(const uint8_t *data, uint16_t length);
void bsp_uart_bt_send_string(const char *text);
bool bsp_uart_bt_take_activity_flag(void);

#endif
