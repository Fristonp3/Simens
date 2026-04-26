#ifndef BOARD_H
#define BOARD_H

#include "gd32f4xx.h"

/* LED pins */
#define BOARD_LED_RUN_PORT       GPIOE
#define BOARD_LED_RUN_CLK        RCU_GPIOE
#define BOARD_LED_RUN_PIN        GPIO_PIN_0

#define BOARD_LED_RECORD_PORT    GPIOE
#define BOARD_LED_RECORD_CLK     RCU_GPIOE
#define BOARD_LED_RECORD_PIN     GPIO_PIN_1

#define BOARD_LED_STATUS_PORT    GPIOE
#define BOARD_LED_STATUS_CLK     RCU_GPIOE
#define BOARD_LED_STATUS_PIN     GPIO_PIN_2

/* Key pins */
#define BOARD_KEY_WAKE_PORT      GPIOA
#define BOARD_KEY_WAKE_CLK       RCU_GPIOA
#define BOARD_KEY_WAKE_PIN       GPIO_PIN_0
#define BOARD_KEY_WAKE_EXTI_LINE  EXTI_0
#define BOARD_KEY_WAKE_IRQN       EXTI0_IRQn

#define BOARD_KEY_MODE_PORT       GPIOC
#define BOARD_KEY_MODE_CLK        RCU_GPIOC
#define BOARD_KEY_MODE_PIN        GPIO_PIN_13
#define BOARD_KEY_MODE_EXTI_LINE  EXTI_13

#define BOARD_KEY_NEXT_PORT       GPIOE
#define BOARD_KEY_NEXT_CLK        RCU_GPIOE
#define BOARD_KEY_NEXT_PIN        GPIO_PIN_3
#define BOARD_KEY_NEXT_EXTI_LINE  EXTI_3

/* UART0 - communication with host */
#define BOARD_UART               USART0
#define BOARD_UART_CLK           RCU_USART0
#define BOARD_UART_TX_PORT       GPIOA
#define BOARD_UART_TX_CLK        RCU_GPIOA
#define BOARD_UART_TX_PIN        GPIO_PIN_9
#define BOARD_UART_TX_AF         GPIO_AF_7
#define BOARD_UART_RX_PORT       GPIOA
#define BOARD_UART_RX_CLK        RCU_GPIOA
#define BOARD_UART_RX_PIN        GPIO_PIN_10
#define BOARD_UART_RX_AF         GPIO_AF_7
#define BOARD_UART_IRQN          USART0_IRQn
#define BOARD_UART_BAUDRATE      115200U

/* ADC input - PC0, ADC012_IN10 */
#define BOARD_ADC_GPIO_PORT      GPIOC
#define BOARD_ADC_GPIO_CLK       RCU_GPIOC
#define BOARD_ADC_GPIO_PIN       GPIO_PIN_0
#define BOARD_ADC_CHANNEL        ADC_CHANNEL_10
#define BOARD_ADC_UNIT           ADC0

/* DAC output - PA4, DAC_OUT0 */
#define BOARD_DAC_GPIO_PORT      GPIOA
#define BOARD_DAC_GPIO_CLK       RCU_GPIOA
#define BOARD_DAC_GPIO_PIN       GPIO_PIN_4
#define BOARD_DAC_UNIT           DAC0
#define BOARD_DAC_OUT            DAC_OUT0

/* I2C0 for OLED */
#define BOARD_I2C_OLED           I2C0
#define BOARD_I2C_OLED_CLK       RCU_I2C0
#define BOARD_I2C_OLED_SCL_PORT  GPIOB
#define BOARD_I2C_OLED_SCL_CLK   RCU_GPIOB
#define BOARD_I2C_OLED_SCL_PIN   GPIO_PIN_6
#define BOARD_I2C_OLED_SCL_AF    GPIO_AF_4
#define BOARD_I2C_OLED_SDA_PORT  GPIOB
#define BOARD_I2C_OLED_SDA_CLK   RCU_GPIOB
#define BOARD_I2C_OLED_SDA_PIN   GPIO_PIN_7
#define BOARD_I2C_OLED_SDA_AF    GPIO_AF_4
#define BOARD_I2C_OLED_ADDRESS   0x78U

/* SPI2 for external Flash */
#define BOARD_SPI_FLASH          SPI2
#define BOARD_SPI_FLASH_CLK      RCU_SPI2
#define BOARD_SPI_FLASH_SCK_PORT GPIOB
#define BOARD_SPI_FLASH_SCK_CLK  RCU_GPIOB
#define BOARD_SPI_FLASH_SCK_PIN  GPIO_PIN_3
#define BOARD_SPI_FLASH_SCK_AF   GPIO_AF_5
#define BOARD_SPI_FLASH_MISO_PORT GPIOB
#define BOARD_SPI_FLASH_MISO_CLK RCU_GPIOB
#define BOARD_SPI_FLASH_MISO_PIN GPIO_PIN_4
#define BOARD_SPI_FLASH_MISO_AF  GPIO_AF_5
#define BOARD_SPI_FLASH_MOSI_PORT GPIOB
#define BOARD_SPI_FLASH_MOSI_CLK RCU_GPIOB
#define BOARD_SPI_FLASH_MOSI_PIN GPIO_PIN_5
#define BOARD_SPI_FLASH_MOSI_AF  GPIO_AF_5
#define BOARD_SPI_FLASH_CS_PORT  GPIOE
#define BOARD_SPI_FLASH_CS_CLK   RCU_GPIOE
#define BOARD_SPI_FLASH_CS_PIN   GPIO_PIN_4

void board_led_init(void);
void board_led_on(uint8_t index);
void board_led_off(uint8_t index);
void board_led_toggle(uint8_t index);

typedef enum {
    BOARD_KEY_WAKEUP = 0,
    BOARD_KEY_MODE,
    BOARD_KEY_NEXT
} board_key_t;

typedef enum {
    BOARD_KEY_MODE_EXTI = 0
} board_key_mode_t;

void board_key_init(board_key_t key, board_key_mode_t mode);
uint8_t board_key_state_get(board_key_t key);

void board_com_init(void);

void board_i2c_oled_init(void);
void board_i2c_oled_write(uint8_t dev_addr, const uint8_t *data, uint16_t length);

void board_spi_flash_init(void);
uint8_t board_spi_flash_transfer_byte(uint8_t byte);
void board_spi_flash_cs_low(void);
void board_spi_flash_cs_high(void);

#endif
