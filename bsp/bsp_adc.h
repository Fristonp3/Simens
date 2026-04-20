#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>

void bsp_adc_init(void);
uint16_t bsp_adc_read_raw(void);
uint16_t bsp_adc_raw_to_mv(uint16_t raw);
int16_t bsp_adc_mv_to_temperature_centi(uint16_t voltage_mv);

#endif
