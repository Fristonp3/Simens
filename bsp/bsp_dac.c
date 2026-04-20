#include "bsp/bsp_dac.h"

#include "gd32f4xx.h"

#define BSP_DAC_GPIO_PORT     GPIOA
#define BSP_DAC_GPIO_CLK      RCU_GPIOA
#define BSP_DAC_GPIO_PIN      GPIO_PIN_4

static uint16_t s_dac_raw;

void bsp_dac_init(void)
{
    rcu_periph_clock_enable(BSP_DAC_GPIO_CLK);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(BSP_DAC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_DAC_GPIO_PIN);

    dac_deinit(DAC0);
    dac_trigger_disable(DAC0, DAC_OUT0);
    dac_output_buffer_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT0);

    bsp_dac_set_raw(2048U);
}

void bsp_dac_set_raw(uint16_t raw)
{
    if(raw > 4095U) {
        raw = 4095U;
    }

    s_dac_raw = raw;
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, raw);
}

uint16_t bsp_dac_get_raw(void)
{
    return s_dac_raw;
}

uint16_t bsp_dac_get_mv(void)
{
    return (uint16_t)(((uint32_t)s_dac_raw * 3300U) / 4095U);
}
