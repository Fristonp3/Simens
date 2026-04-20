#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"

void bsp_flash_init(void);
void bsp_flash_erase_all(void);
bool bsp_flash_append_record(const sample_record_t *record);
bool bsp_flash_read_record(uint16_t index, sample_record_t *record);
uint16_t bsp_flash_record_count(void);
uint16_t bsp_flash_record_capacity(void);

#endif
