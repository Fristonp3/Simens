#include "bl_flash.h"

#include <string.h>

#include "common/crc16.h"

#define BL_FLASH_STAGE_SIZE 1024U

static uint8_t s_stage[BL_FLASH_STAGE_SIZE];
static uint32_t s_staged_size;

void bl_flash_reset(void)
{
    memset(s_stage, 0xFF, sizeof(s_stage));
    s_staged_size = 0U;
}

bool bl_flash_stage_chunk(uint32_t address, const uint8_t *data, uint16_t length)
{
    if((address + length) > BL_FLASH_STAGE_SIZE) {
        return false;
    }

    memcpy(&s_stage[address], data, length);
    if((address + length) > s_staged_size) {
        s_staged_size = address + length;
    }
    return true;
}

uint32_t bl_flash_staged_size(void)
{
    return s_staged_size;
}

uint16_t bl_flash_crc16(void)
{
    return crc16_modbus(s_stage, s_staged_size);
}
