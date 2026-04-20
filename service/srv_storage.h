#ifndef SRV_STORAGE_H
#define SRV_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"

void srv_storage_init(void);
bool srv_storage_append(const sample_record_t *record);
bool srv_storage_read(uint16_t index, sample_record_t *record);
uint16_t srv_storage_read_range(uint16_t start, uint8_t max_count, sample_record_t *records);
void srv_storage_clear(void);
uint16_t srv_storage_count(void);
uint16_t srv_storage_capacity(void);
bool srv_storage_is_full(void);

#endif
