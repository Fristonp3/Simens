#include "bsp/bsp_adc.h"

#include "gd32f4xx.h"
#include "systick.h"

#define BSP_ADC_GPIO_PORT        GPIOC
#define BSP_ADC_GPIO_CLK         RCU_GPIOC
#define BSP_ADC_GPIO_PIN         GPIO_PIN_0
#define BSP_ADC_CHANNEL          ADC_CHANNEL_10

void bsp_adc_init(void)
{
    rcu_periph_clock_enable(BSP_ADC_GPIO_CLK);
    rcu_periph_clock_enable(RCU_ADC0);

    gpio_mode_set(BSP_ADC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BSP_ADC_GPIO_PIN);

    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_resolution_config(ADC0, ADC_RESOLUTION_12B);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1U);
    adc_routine_channel_config(ADC0, 0U, BSP_ADC_CHANNEL, ADC_SAMPLETIME_480);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(ADC0);

    delay_1ms(1U);
}

uint16_t bsp_adc_read_raw(void)
{
    adc_flag_clear(ADC0, ADC_FLAG_EOC);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    while(RESET == adc_flag_get(ADC0, ADC_FLAG_EOC)) {
    }

    return adc_routine_data_read(ADC0);
}

uint16_t bsp_adc_raw_to_mv(uint16_t raw)
{
    return (uint16_t)(((uint32_t)raw * 3300U) / 4095U);
}

int16_t bsp_adc_mv_to_temperature_centi(uint16_t voltage_mv)
{
    if(voltage_mv <= 500U) {
        return 0;
    }

    if(voltage_mv >= 2500U) {
        return 20000;
    }

    return (int16_t)(((int32_t)voltage_mv - 500) * 10);
}
