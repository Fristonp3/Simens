#ifndef BL_FLASH_H
#define BL_FLASH_H

#include <stdbool.h>
#include <stdint.h>

void bl_flash_reset(void);
bool bl_flash_stage_chunk(uint32_t address, const uint8_t *data, uint16_t length);
uint32_t bl_flash_staged_size(void);
uint16_t bl_flash_crc16(void);

#endif
