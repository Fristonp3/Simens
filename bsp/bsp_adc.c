#include "bsp/bsp_adc.h"

#include "board.h"
#include "gd32f4xx.h"

void bsp_adc_init(void)
{
    rcu_periph_clock_enable(BOARD_ADC_GPIO_CLK);
    rcu_periph_clock_enable(RCU_ADC0);

    gpio_mode_set(BOARD_ADC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BOARD_ADC_GPIO_PIN);

    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_special_function_config(BOARD_ADC_UNIT, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(BOARD_ADC_UNIT, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(BOARD_ADC_UNIT, ADC_DATAALIGN_RIGHT);
    adc_resolution_config(BOARD_ADC_UNIT, ADC_RESOLUTION_12B);
    adc_channel_length_config(BOARD_ADC_UNIT, ADC_ROUTINE_CHANNEL, 1U);
    adc_routine_channel_config(BOARD_ADC_UNIT, 0U, BOARD_ADC_CHANNEL, ADC_SAMPLETIME_480);
    adc_external_trigger_config(BOARD_ADC_UNIT, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(BOARD_ADC_UNIT);

    {
        volatile uint32_t i;
        for (i = 0U; i < 10000U; i++) { __asm volatile("nop"); }
    }
}

uint16_t bsp_adc_read_raw(void)
{
    adc_flag_clear(BOARD_ADC_UNIT, ADC_FLAG_EOC);
    adc_software_trigger_enable(BOARD_ADC_UNIT, ADC_ROUTINE_CHANNEL);
    while (RESET == adc_flag_get(BOARD_ADC_UNIT, ADC_FLAG_EOC)) {
    }

    return adc_routine_data_read(BOARD_ADC_UNIT);
}

uint16_t bsp_adc_raw_to_mv(uint16_t raw)
{
    return (uint16_t)(((uint32_t)raw * 3300U) / 4095U);
}

int16_t bsp_adc_mv_to_temperature_centi(uint16_t voltage_mv)
{
    if (voltage_mv <= 500U) {
        return 0;
    }

    if (voltage_mv >= 2500U) {
        return 20000;
    }

    /* PT100: 0.5V->0C, 2.5V->200C, linear: temp_cC = (mv-500)*10 */
    return (int16_t)(((int32_t)voltage_mv - 500) * 10);
}
