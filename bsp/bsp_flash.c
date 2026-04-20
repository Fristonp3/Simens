#include "bsp/bsp_flash.h"

#include <string.h>

#include "gd32f4xx.h"

#define BSP_FLASH_BKPSRAM_SIZE      4096U
#define BSP_FLASH_MAGIC             0x53464C48UL
#define BSP_FLASH_VERSION           0x0001U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t record_count;
} bsp_flash_header_t;

#define BSP_FLASH_RECORD_CAPACITY   ((BSP_FLASH_BKPSRAM_SIZE - sizeof(bsp_flash_header_t)) / sizeof(sample_record_t))
#define BSP_FLASH_HEADER_PTR        ((bsp_flash_header_t *)(uintptr_t)BKPSRAM_BASE)
#define BSP_FLASH_RECORD_PTR        ((sample_record_t *)(uintptr_t)(BKPSRAM_BASE + sizeof(bsp_flash_header_t)))

static void bsp_flash_prepare_domain(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    rcu_periph_clock_enable(RCU_BKPSRAM);
}

static bool bsp_flash_header_valid(const bsp_flash_header_t *header)
{
    return ((header->magic == BSP_FLASH_MAGIC) &&
            (header->version == BSP_FLASH_VERSION) &&
            (header->record_count <= BSP_FLASH_RECORD_CAPACITY));
}

void bsp_flash_init(void)
{
    bsp_flash_prepare_domain();

    if(!bsp_flash_header_valid(BSP_FLASH_HEADER_PTR)) {
        bsp_flash_erase_all();
    }
}

void bsp_flash_erase_all(void)
{
    bsp_flash_prepare_domain();
    memset((void *)(uintptr_t)BKPSRAM_BASE, 0, BSP_FLASH_BKPSRAM_SIZE);
    BSP_FLASH_HEADER_PTR->magic = BSP_FLASH_MAGIC;
    BSP_FLASH_HEADER_PTR->version = BSP_FLASH_VERSION;
    BSP_FLASH_HEADER_PTR->record_count = 0U;
}

bool bsp_flash_append_record(const sample_record_t *record)
{
    uint16_t count;

    bsp_flash_prepare_domain();
    count = bsp_flash_record_count();
    if(count >= BSP_FLASH_RECORD_CAPACITY) {
        return false;
    }

    BSP_FLASH_RECORD_PTR[count] = *record;
    BSP_FLASH_HEADER_PTR->record_count = (uint16_t)(count + 1U);
    return true;
}

bool bsp_flash_read_record(uint16_t index, sample_record_t *record)
{
    if(index >= bsp_flash_record_count()) {
        return false;
    }

    *record = BSP_FLASH_RECORD_PTR[index];
    return true;
}

uint16_t bsp_flash_record_count(void)
{
    if(!bsp_flash_header_valid(BSP_FLASH_HEADER_PTR)) {
        return 0U;
    }

    return BSP_FLASH_HEADER_PTR->record_count;
}

uint16_t bsp_flash_record_capacity(void)
{
    return BSP_FLASH_RECORD_CAPACITY;
}
