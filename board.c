#include "board.h"

/* -------------------------------------------------------------------------- */
/* LED                                                                        */
/* -------------------------------------------------------------------------- */
static const struct {
    uint32_t clk;
    uint32_t port;
    uint16_t pin;
} s_led_map[3] = {
    {BOARD_LED_RUN_CLK,    (uint32_t)BOARD_LED_RUN_PORT,    BOARD_LED_RUN_PIN},
    {BOARD_LED_RECORD_CLK, (uint32_t)BOARD_LED_RECORD_PORT, BOARD_LED_RECORD_PIN},
    {BOARD_LED_STATUS_CLK, (uint32_t)BOARD_LED_STATUS_PORT, BOARD_LED_STATUS_PIN}
};

void board_led_init(void)
{
    uint8_t i;
    for (i = 0U; i < 3U; i++) {
        rcu_periph_clock_enable(s_led_map[i].clk);
        gpio_mode_set((uint32_t)s_led_map[i].port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, s_led_map[i].pin);
        gpio_output_options_set((uint32_t)s_led_map[i].port, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, s_led_map[i].pin);
        gpio_bit_reset((uint32_t)s_led_map[i].port, s_led_map[i].pin);
    }
}

void board_led_on(uint8_t index)
{
    if (index < 3U) {
        gpio_bit_set((uint32_t)s_led_map[index].port, s_led_map[index].pin);
    }
}

void board_led_off(uint8_t index)
{
    if (index < 3U) {
        gpio_bit_reset((uint32_t)s_led_map[index].port, s_led_map[index].pin);
    }
}

void board_led_toggle(uint8_t index)
{
    if (index < 3U) {
        gpio_bit_toggle((uint32_t)s_led_map[index].port, s_led_map[index].pin);
    }
}

/* -------------------------------------------------------------------------- */
/* Keys                                                                       */
/* -------------------------------------------------------------------------- */
static const struct {
    uint32_t clk;
    uint32_t port;
    uint16_t pin;
    uint32_t exti_line;
    uint32_t exti_port;
    uint32_t irqn;
} s_key_map[3] = {
    {BOARD_KEY_WAKE_CLK, (uint32_t)BOARD_KEY_WAKE_PORT, BOARD_KEY_WAKE_PIN, BOARD_KEY_WAKE_EXTI_LINE, EXTI_SOURCE_GPIOA, BOARD_KEY_WAKE_IRQN},
    {BOARD_KEY_MODE_CLK, (uint32_t)BOARD_KEY_MODE_PORT, BOARD_KEY_MODE_PIN, BOARD_KEY_MODE_EXTI_LINE, EXTI_SOURCE_GPIOC, EXTI10_15_IRQn},
    {BOARD_KEY_NEXT_CLK, (uint32_t)BOARD_KEY_NEXT_PORT, BOARD_KEY_NEXT_PIN, BOARD_KEY_NEXT_EXTI_LINE, EXTI_SOURCE_GPIOE, EXTI3_IRQn}
};

void board_key_init(board_key_t key, board_key_mode_t mode)
{
    if (key >= 3U) return;
    (void)mode;

    rcu_periph_clock_enable(s_key_map[key].clk);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(s_key_map[key].port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, s_key_map[key].pin);

    syscfg_exti_line_config(s_key_map[key].exti_port, s_key_map[key].exti_line);
    exti_init(s_key_map[key].exti_line, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_enable(s_key_map[key].exti_line);

    nvic_irq_enable(s_key_map[key].irqn, 1U, 0U);
}

uint8_t board_key_state_get(board_key_t key)
{
    if (key >= 3U) return SET;
    /* Returns RESET when pressed (active low) */
    return gpio_input_bit_get(s_key_map[key].port, s_key_map[key].pin);
}

/* -------------------------------------------------------------------------- */
/* UART0                                                                      */
/* -------------------------------------------------------------------------- */
void board_com_init(void)
{
    rcu_periph_clock_enable(BOARD_UART_TX_CLK);
    rcu_periph_clock_enable(BOARD_UART_RX_CLK);
    rcu_periph_clock_enable(BOARD_UART_CLK);

    gpio_af_set(BOARD_UART_TX_PORT, BOARD_UART_TX_AF, BOARD_UART_TX_PIN);
    gpio_mode_set(BOARD_UART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BOARD_UART_TX_PIN);
    gpio_output_options_set(BOARD_UART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BOARD_UART_TX_PIN);

    gpio_af_set(BOARD_UART_RX_PORT, BOARD_UART_RX_AF, BOARD_UART_RX_PIN);
    gpio_mode_set(BOARD_UART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BOARD_UART_RX_PIN);

    usart_deinit(BOARD_UART);
    usart_baudrate_set(BOARD_UART, BOARD_UART_BAUDRATE);
    usart_word_length_set(BOARD_UART, USART_WL_8BIT);
    usart_stop_bit_set(BOARD_UART, USART_STB_1BIT);
    usart_parity_config(BOARD_UART, USART_PM_NONE);
    usart_hardware_flow_rts_config(BOARD_UART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(BOARD_UART, USART_CTS_DISABLE);
    usart_receive_config(BOARD_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(BOARD_UART, USART_TRANSMIT_ENABLE);
    usart_enable(BOARD_UART);
}

/* -------------------------------------------------------------------------- */
/* I2C for OLED                                                               */
/* -------------------------------------------------------------------------- */
void board_i2c_oled_init(void)
{
    rcu_periph_clock_enable(BOARD_I2C_OLED_SCL_CLK);
    rcu_periph_clock_enable(BOARD_I2C_OLED_SDA_CLK);
    rcu_periph_clock_enable(BOARD_I2C_OLED_CLK);

    gpio_af_set(BOARD_I2C_OLED_SCL_PORT, BOARD_I2C_OLED_SCL_AF, BOARD_I2C_OLED_SCL_PIN);
    gpio_mode_set(BOARD_I2C_OLED_SCL_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BOARD_I2C_OLED_SCL_PIN);
    gpio_output_options_set(BOARD_I2C_OLED_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, BOARD_I2C_OLED_SCL_PIN);

    gpio_af_set(BOARD_I2C_OLED_SDA_PORT, BOARD_I2C_OLED_SDA_AF, BOARD_I2C_OLED_SDA_PIN);
    gpio_mode_set(BOARD_I2C_OLED_SDA_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BOARD_I2C_OLED_SDA_PIN);
    gpio_output_options_set(BOARD_I2C_OLED_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, BOARD_I2C_OLED_SDA_PIN);

    i2c_deinit(BOARD_I2C_OLED);
    i2c_clock_config(BOARD_I2C_OLED, 400000U, I2C_DTCY_2);
    i2c_mode_addr_config(BOARD_I2C_OLED, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0U);
    i2c_enable(BOARD_I2C_OLED);
    i2c_ack_config(BOARD_I2C_OLED, I2C_ACK_ENABLE);
}

void board_i2c_oled_write(uint8_t dev_addr, const uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint32_t timeout;

    /* Wait for I2C idle */
    timeout = 100000U;
    while (i2c_flag_get(BOARD_I2C_OLED, I2C_FLAG_I2CBSY) && timeout) {
        timeout--;
    }

    i2c_start_on_bus(BOARD_I2C_OLED);
    while (!i2c_flag_get(BOARD_I2C_OLED, I2C_FLAG_SBSEND));

    i2c_master_addressing(BOARD_I2C_OLED, dev_addr, I2C_TRANSMITTER);
    while (!i2c_flag_get(BOARD_I2C_OLED, I2C_FLAG_ADDSEND));
    i2c_flag_clear(BOARD_I2C_OLED, I2C_FLAG_ADDSEND);

    for (i = 0U; i < length; i++) {
        while (!i2c_flag_get(BOARD_I2C_OLED, I2C_FLAG_TBE));
        i2c_data_transmit(BOARD_I2C_OLED, data[i]);
    }

    while (!i2c_flag_get(BOARD_I2C_OLED, I2C_FLAG_TBE));
    i2c_stop_on_bus(BOARD_I2C_OLED);
    while (I2C_CTL0(BOARD_I2C_OLED) & 0x0200U);
}

/* -------------------------------------------------------------------------- */
/* SPI for external Flash                                                     */
/* -------------------------------------------------------------------------- */
void board_spi_flash_init(void)
{
    rcu_periph_clock_enable(BOARD_SPI_FLASH_SCK_CLK);
    rcu_periph_clock_enable(BOARD_SPI_FLASH_MISO_CLK);
    rcu_periph_clock_enable(BOARD_SPI_FLASH_MOSI_CLK);
    rcu_periph_clock_enable(BOARD_SPI_FLASH_CS_CLK);
    rcu_periph_clock_enable(BOARD_SPI_FLASH_CLK);

    gpio_af_set(BOARD_SPI_FLASH_SCK_PORT, BOARD_SPI_FLASH_SCK_AF, BOARD_SPI_FLASH_SCK_PIN);
    gpio_mode_set(BOARD_SPI_FLASH_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BOARD_SPI_FLASH_SCK_PIN);
    gpio_output_options_set(BOARD_SPI_FLASH_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BOARD_SPI_FLASH_SCK_PIN);

    gpio_af_set(BOARD_SPI_FLASH_MISO_PORT, BOARD_SPI_FLASH_MISO_AF, BOARD_SPI_FLASH_MISO_PIN);
    gpio_mode_set(BOARD_SPI_FLASH_MISO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BOARD_SPI_FLASH_MISO_PIN);

    gpio_af_set(BOARD_SPI_FLASH_MOSI_PORT, BOARD_SPI_FLASH_MOSI_AF, BOARD_SPI_FLASH_MOSI_PIN);
    gpio_mode_set(BOARD_SPI_FLASH_MOSI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BOARD_SPI_FLASH_MOSI_PIN);
    gpio_output_options_set(BOARD_SPI_FLASH_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BOARD_SPI_FLASH_MOSI_PIN);

    /* CS as GPIO output */
    gpio_mode_set(BOARD_SPI_FLASH_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BOARD_SPI_FLASH_CS_PIN);
    gpio_output_options_set(BOARD_SPI_FLASH_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BOARD_SPI_FLASH_CS_PIN);
    board_spi_flash_cs_high();

    spi_parameter_struct spi_cfg;
    spi_struct_para_init(&spi_cfg);
    spi_cfg.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_cfg.device_mode = SPI_MASTER;
    spi_cfg.frame_size = SPI_FRAMESIZE_8BIT;
    spi_cfg.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_cfg.nss = SPI_NSS_SOFT;
    spi_cfg.prescale = SPI_PSC_8;
    spi_cfg.endian = SPI_ENDIAN_MSB;
    spi_init(BOARD_SPI_FLASH, &spi_cfg);
    spi_enable(BOARD_SPI_FLASH);
}

uint8_t board_spi_flash_transfer_byte(uint8_t byte)
{
    while (RESET == spi_i2s_flag_get(BOARD_SPI_FLASH, SPI_FLAG_TBE));
    spi_i2s_data_transmit(BOARD_SPI_FLASH, byte);
    while (RESET == spi_i2s_flag_get(BOARD_SPI_FLASH, SPI_FLAG_RBNE));
    return (uint8_t)spi_i2s_data_receive(BOARD_SPI_FLASH);
}

void board_spi_flash_cs_low(void)
{
    gpio_bit_reset(BOARD_SPI_FLASH_CS_PORT, BOARD_SPI_FLASH_CS_PIN);
}

void board_spi_flash_cs_high(void)
{
    gpio_bit_set(BOARD_SPI_FLASH_CS_PORT, BOARD_SPI_FLASH_CS_PIN);
}
