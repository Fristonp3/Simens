#ifndef BL_FLASH_H
#define BL_FLASH_H

#include <stdbool.h>
#include <stdint.h>

void bl_flash_reset(void);
void bl_flash_erase_app(void);
bool bl_flash_write_chunk(uint32_t addr, const uint8_t *data, uint16_t length);
bool bl_flash_verify_crc(uint16_t expected_crc);
void bl_flash_mark_complete(void);
bool bl_flash_is_complete(void);
uint32_t bl_flash_staged_size(void);
uint16_t bl_flash_crc16(void);

#endif
