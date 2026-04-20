#include "service/srv_settings.h"

#include <string.h>

#include "bsp/bsp_rtc.h"
#include "common/crc16.h"

#define SETTINGS_MAGIC           0x43464731UL
#define SETTINGS_BKP_MAGIC       1U
#define SETTINGS_BKP_CRC         2U
#define SETTINGS_BKP_SAMPLE      3U
#define SETTINGS_BKP_DISPLAY     4U
#define SETTINGS_BKP_RECORD      5U
#define SETTINGS_BKP_DAC_PAGE    6U

static bool srv_settings_valid_period(uint32_t value, uint32_t min, uint32_t max)
{
    return ((value >= min) && (value <= max));
}

static bool srv_settings_validate(const system_config_t *config)
{
    if(!srv_settings_valid_period(config->sample_period_ms, 50U, 5000U)) {
        return false;
    }
    if(!srv_settings_valid_period(config->display_period_ms, 50U, 5000U)) {
        return false;
    }
    if(!srv_settings_valid_period(config->record_period_ms, 100U, 10000U)) {
        return false;
    }
    if(config->dac_raw > 4095U) {
        return false;
    }
    if(config->display_page > APP_PAGE_EXTENDED) {
        return false;
    }

    return true;
}

bool srv_settings_load(system_config_t *config)
{
    uint32_t stored_magic = bsp_rtc_backup_read(SETTINGS_BKP_MAGIC);
    uint32_t stored_crc = bsp_rtc_backup_read(SETTINGS_BKP_CRC);
    uint32_t raw_words[4];
    uint16_t computed_crc;
    system_config_t loaded;

    if(stored_magic != SETTINGS_MAGIC) {
        return false;
    }

    raw_words[0] = bsp_rtc_backup_read(SETTINGS_BKP_SAMPLE);
    raw_words[1] = bsp_rtc_backup_read(SETTINGS_BKP_DISPLAY);
    raw_words[2] = bsp_rtc_backup_read(SETTINGS_BKP_RECORD);
    raw_words[3] = bsp_rtc_backup_read(SETTINGS_BKP_DAC_PAGE);
    computed_crc = crc16_modbus((const uint8_t *)raw_words, sizeof(raw_words));

    if(stored_crc != computed_crc) {
        return false;
    }

    loaded.sample_period_ms = raw_words[0];
    loaded.display_period_ms = raw_words[1];
    loaded.record_period_ms = raw_words[2];
    loaded.dac_raw = (uint16_t)(raw_words[3] & 0x0FFFU);
    loaded.display_page = (app_page_t)((raw_words[3] >> 16U) & 0x01U);

    if(!srv_settings_validate(&loaded)) {
        return false;
    }

    *config = loaded;
    return true;
}

void srv_settings_save(const system_config_t *config)
{
    uint32_t raw_words[4];
    uint16_t crc;

    if(!srv_settings_validate(config)) {
        return;
    }

    raw_words[0] = config->sample_period_ms;
    raw_words[1] = config->display_period_ms;
    raw_words[2] = config->record_period_ms;
    raw_words[3] = ((uint32_t)config->display_page << 16U) | config->dac_raw;
    crc = crc16_modbus((const uint8_t *)raw_words, sizeof(raw_words));

    bsp_rtc_backup_write(SETTINGS_BKP_MAGIC, SETTINGS_MAGIC);
    bsp_rtc_backup_write(SETTINGS_BKP_SAMPLE, raw_words[0]);
    bsp_rtc_backup_write(SETTINGS_BKP_DISPLAY, raw_words[1]);
    bsp_rtc_backup_write(SETTINGS_BKP_RECORD, raw_words[2]);
    bsp_rtc_backup_write(SETTINGS_BKP_DAC_PAGE, raw_words[3]);
    bsp_rtc_backup_write(SETTINGS_BKP_CRC, crc);
}
