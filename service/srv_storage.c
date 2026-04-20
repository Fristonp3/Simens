#include "service/srv_storage.h"

#include "bsp/bsp_flash.h"

void srv_storage_init(void)
{
    bsp_flash_init();
}

bool srv_storage_append(const sample_record_t *record)
{
    return bsp_flash_append_record(record);
}

bool srv_storage_read(uint16_t index, sample_record_t *record)
{
    return bsp_flash_read_record(index, record);
}

uint16_t srv_storage_read_range(uint16_t start, uint8_t max_count, sample_record_t *records)
{
    uint16_t count = 0U;
    uint16_t total = srv_storage_count();

    while((count < max_count) && ((start + count) < total)) {
        if(!srv_storage_read((uint16_t)(start + count), &records[count])) {
            break;
        }
        ++count;
    }

    return count;
}

void srv_storage_clear(void)
{
    bsp_flash_erase_all();
}

uint16_t srv_storage_count(void)
{
    return bsp_flash_record_count();
}

uint16_t srv_storage_capacity(void)
{
    return bsp_flash_record_capacity();
}

bool srv_storage_is_full(void)
{
    return (srv_storage_count() >= srv_storage_capacity());
}
