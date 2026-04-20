#ifndef BSP_RTC_H
#define BSP_RTC_H

#include <stdint.h>

#include "common/project_types.h"

void bsp_rtc_init(void);
void bsp_rtc_set_datetime(const rtc_datetime_t *datetime);
void bsp_rtc_get_datetime(rtc_datetime_t *datetime);
uint32_t bsp_rtc_get_epoch_seconds(void);
uint32_t bsp_rtc_backup_read(uint8_t index);
void bsp_rtc_backup_write(uint8_t index, uint32_t value);

#endif
