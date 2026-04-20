#ifndef BSP_DAC_H
#define BSP_DAC_H

#include <stdint.h>

void bsp_dac_init(void);
void bsp_dac_set_raw(uint16_t raw);
uint16_t bsp_dac_get_raw(void);
uint16_t bsp_dac_get_mv(void);

#endif
