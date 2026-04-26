#include "bsp/bsp_uart.h"

#include "board.h"
#include "common/ringbuffer.h"

#define BSP_UART_RX_BUFFER_SIZE 256U

static uint8_t s_rx_storage[BSP_UART_RX_BUFFER_SIZE];
static ringbuffer_t s_rx_buffer;
static volatile bool s_activity_flag;

void bsp_uart_init(void)
{
    board_com_init();
    ringbuffer_init(&s_rx_buffer, s_rx_storage, BSP_UART_RX_BUFFER_SIZE);
    s_activity_flag = false;

    usart_interrupt_enable(BOARD_UART, USART_INT_RBNE);
    nvic_irq_enable(BOARD_UART_IRQN, 1U, 0U);
}

void bsp_uart_irq_handler(void)
{
    while (SET == usart_interrupt_flag_get(BOARD_UART, USART_INT_FLAG_RBNE)) {
        uint8_t data = (uint8_t)usart_data_receive(BOARD_UART);
        (void)ringbuffer_push(&s_rx_buffer, data);
        s_activity_flag = true;
    }
}

bool bsp_uart_read_byte(uint8_t *byte)
{
    return ringbuffer_pop(&s_rx_buffer, byte);
}

uint16_t bsp_uart_available(void)
{
    return ringbuffer_count(&s_rx_buffer);
}

void bsp_uart_send_bytes(const uint8_t *data, uint16_t length)
{
    uint16_t index;

    for (index = 0U; index < length; ++index) {
        usart_data_transmit(BOARD_UART, data[index]);
        while (RESET == usart_flag_get(BOARD_UART, USART_FLAG_TBE)) {
        }
    }
}

void bsp_uart_send_string(const char *text)
{
    while (*text != '\0') {
        usart_data_transmit(BOARD_UART, (uint8_t)(*text));
        while (RESET == usart_flag_get(BOARD_UART, USART_FLAG_TBE)) {
        }
        ++text;
    }
}

bool bsp_uart_take_activity_flag(void)
{
    bool active = s_activity_flag;
    s_activity_flag = false;
    return active;
}
