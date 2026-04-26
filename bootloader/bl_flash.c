#include "bl_flash.h"

#include <string.h>

#include "common/crc16.h"
#include "gd32f4xx.h"

/* Memory layout for GD32F470VET6 (512KB flash):
   Bootloader: 0x08000000 - 0x08007FFF (32KB, pages 0-1: 2 x 16KB)
   Application: 0x08008000 - 0x0807FFFF (480KB)
*/
#define BL_APP_START_ADDR     0x08008000U
#define BL_APP_MAX_SIZE       0x00078000U  /* 480KB */

static uint32_t s_app_size;
static uint16_t s_expected_crc;
static bool s_complete;

/* Page size for GD32F4xx:
   For 512KB devices: pages 0-1 are 16KB, pages 2-3 are 16KB,
   pages 4+ are 2KB each.
   Application starts at page 2 (0x08008000), so we erase from page 2 onward. */
#define BL_PAGE_SIZE_LARGE    0x4000U   /* 16KB */
#define BL_PAGE_SIZE_SMALL    0x0800U   /* 2KB */
#define BL_LARGE_PAGES        4U        /* Pages 0-3 are 16KB */
#define BL_FIRST_APP_PAGE     2U        /* Start erasing from page 2 */

void bl_flash_reset(void)
{
    s_app_size = 0U;
    s_expected_crc = 0U;
    s_complete = false;
}

void bl_flash_erase_app(void)
{
    uint32_t page_addr;
    uint8_t page;

    fmc_unlock();

    for (page = BL_FIRST_APP_PAGE; page < 256U; page++) {
        page_addr = 0x08000000U;
        if (page < BL_LARGE_PAGES) {
            page_addr += (uint32_t)page * BL_PAGE_SIZE_LARGE;
        } else {
            page_addr += (uint32_t)BL_LARGE_PAGES * BL_PAGE_SIZE_LARGE;
            page_addr += (uint32_t)(page - BL_LARGE_PAGES) * BL_PAGE_SIZE_SMALL;
        }

        /* Stop at end of flash (512KB) */
        if (page_addr >= 0x08080000U) { break; }

        if (fmc_page_erase(page_addr) != FMC_READY) {
            break;
        }
    }

    fmc_lock();
    s_app_size = 0U;
    s_complete = false;
}

bool bl_flash_write_chunk(uint32_t addr, const uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint32_t write_addr;

    if (addr < BL_APP_START_ADDR) { return false; }
    if ((addr + length) > (BL_APP_START_ADDR + BL_APP_MAX_SIZE)) { return false; }

    /* On GD32F4xx, FMC writes 32-bit words */
    fmc_unlock();

    write_addr = addr;
    for (i = 0U; (i + 4U) <= length; i += 4U) {
        uint32_t word = ((uint32_t)data[i]) |
                        ((uint32_t)data[i + 1U] << 8U) |
                        ((uint32_t)data[i + 2U] << 16U) |
                        ((uint32_t)data[i + 3U] << 24U);
        if (fmc_word_program(write_addr, word) != FMC_READY) {
            fmc_lock();
            return false;
        }
        write_addr += 4U;
    }

    /* Handle remaining bytes (0-3) with a padded word write */
    if ((length % 4U) != 0U) {
        uint32_t word = 0xFFFFFFFFU;
        uint16_t remaining = length - i;

        for (uint16_t j = 0U; j < remaining; j++) {
            ((uint8_t *)&word)[j] = data[i + j];
        }

        if (fmc_word_program(write_addr, word) != FMC_READY) {
            fmc_lock();
            return false;
        }
    }

    fmc_lock();

    /* Track total data written */
    if ((addr - BL_APP_START_ADDR + length) > s_app_size) {
        s_app_size = addr - BL_APP_START_ADDR + length;
    }

    return true;
}

bool bl_flash_verify_crc(uint16_t expected_crc)
{
    uint16_t computed;
    uint32_t size_to_check;

    if (s_app_size == 0U) { return false; }

    /* Compute CRC16 over the written application data */
    computed = crc16_modbus((const uint8_t *)BL_APP_START_ADDR, (size_t)s_app_size);

    s_expected_crc = expected_crc;
    return (computed == expected_crc);
}

void bl_flash_mark_complete(void)
{
    s_complete = true;
}

bool bl_flash_is_complete(void)
{
    return s_complete;
}

uint32_t bl_flash_staged_size(void)
{
    return s_app_size;
}

uint16_t bl_flash_crc16(void)
{
    if (s_app_size == 0U) { return 0U; }
    return crc16_modbus((const uint8_t *)BL_APP_START_ADDR, (size_t)s_app_size);
}
