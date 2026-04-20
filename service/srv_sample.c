#include "service/srv_sample.h"

#include "bsp/bsp_adc.h"
#include "bsp/bsp_dac.h"
#include "bsp/bsp_rtc.h"
#include "common/filter.h"

static ema_filter_u16_t s_adc_filter;
static sample_record_t s_latest_sample;

void srv_sample_init(void)
{
    filter_ema_u16_init(&s_adc_filter, 64U);
    s_latest_sample.raw_adc = 0U;
    s_latest_sample.voltage_mv = 0U;
    s_latest_sample.temperature_centi_c = 0;
    s_latest_sample.dac_raw = 0U;
    s_latest_sample.status = 0U;
}

const sample_record_t *srv_sample_capture(void)
{
    uint16_t raw = bsp_adc_read_raw();
    uint16_t filtered_raw = filter_ema_u16_apply(&s_adc_filter, raw);
    uint16_t voltage_mv = bsp_adc_raw_to_mv(filtered_raw);
    uint8_t status = 0U;

    if(voltage_mv < 450U) {
        status |= SAMPLE_STATUS_SENSOR_SHORT;
    } else if(voltage_mv > 2550U) {
        status |= SAMPLE_STATUS_SENSOR_OPEN;
    }

    bsp_rtc_get_datetime(&s_latest_sample.timestamp);
    s_latest_sample.raw_adc = filtered_raw;
    s_latest_sample.voltage_mv = voltage_mv;
    s_latest_sample.temperature_centi_c = bsp_adc_mv_to_temperature_centi(voltage_mv);
    s_latest_sample.dac_raw = bsp_dac_get_raw();
    s_latest_sample.status = status;

    return &s_latest_sample;
}

const sample_record_t *srv_sample_latest(void)
{
    return &s_latest_sample;
}
