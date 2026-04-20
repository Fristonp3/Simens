#include "bsp/bsp_rtc.h"

#include "common/crc16.h"
#include "gd32f4xx.h"

#define BSP_RTC_INIT_MAGIC         0x47324D43UL
#define BSP_RTC_BACKUP_REG_BASE    0x50U
#define BSP_RTC_BACKUP_REG_COUNT   20U

static uint8_t rtc_bin_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static uint8_t rtc_bcd_to_bin(uint8_t value)
{
    return (uint8_t)(((value >> 4U) * 10U) + (value & 0x0FU));
}

static bool rtc_is_leap_year(uint16_t year)
{
    return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U)));
}

static uint8_t rtc_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days_per_month[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };

    uint8_t days = days_per_month[month - 1U];
    if((month == 2U) && rtc_is_leap_year(year)) {
        days = 29U;
    }

    return days;
}

static uint8_t rtc_compute_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t adjust = (month < 3U) ? 1U : 0U;
    uint32_t y = year - adjust;
    uint32_t m = (month < 3U) ? (month + 12U) : month;
    uint32_t k = y % 100U;
    uint32_t j = y / 100U;
    uint32_t h = (day + ((13U * (m + 1U)) / 5U) + k + (k / 4U) + (j / 4U) + (5U * j)) % 7U;

    return (uint8_t)(((h + 5U) % 7U) + 1U);
}

static bool rtc_datetime_is_valid(const rtc_datetime_t *datetime)
{
    if((datetime->year < 2000U) || (datetime->year > 2099U)) {
        return false;
    }
    if((datetime->month < 1U) || (datetime->month > 12U)) {
        return false;
    }
    if((datetime->day < 1U) || (datetime->day > rtc_days_in_month(datetime->year, datetime->month))) {
        return false;
    }
    if((datetime->hour > 23U) || (datetime->minute > 59U) || (datetime->second > 59U)) {
        return false;
    }

    return true;
}

static uint32_t rtc_datetime_to_epoch(const rtc_datetime_t *datetime)
{
    uint32_t days = 0U;
    uint16_t year;
    uint8_t month;

    for(year = 2000U; year < datetime->year; ++year) {
        days += rtc_is_leap_year(year) ? 366U : 365U;
    }

    for(month = 1U; month < datetime->month; ++month) {
        days += rtc_days_in_month(datetime->year, month);
    }

    days += (uint32_t)(datetime->day - 1U);

    return (((days * 24U) + datetime->hour) * 60U + datetime->minute) * 60U + datetime->second;
}

static void rtc_apply_default_time(void)
{
    rtc_datetime_t default_time = {2026U, 1U, 1U, 0U, 0U, 0U, 4U};
    bsp_rtc_set_datetime(&default_time);
}

static uint32_t rtc_backup_reg_addr(uint8_t index)
{
    return RTC + BSP_RTC_BACKUP_REG_BASE + ((uint32_t)index * 4U);
}

void bsp_rtc_init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    if(RTC_BKP0 != BSP_RTC_INIT_MAGIC) {
        rcu_osci_on(RCU_IRC32K);
        (void)rcu_osci_stab_wait(RCU_IRC32K);
        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
        rcu_periph_clock_enable(RCU_RTC);
        (void)rtc_register_sync_wait();
        rtc_apply_default_time();
        RTC_BKP0 = BSP_RTC_INIT_MAGIC;
    } else {
        rcu_periph_clock_enable(RCU_RTC);
        (void)rtc_register_sync_wait();
    }
}

void bsp_rtc_set_datetime(const rtc_datetime_t *datetime)
{
    rtc_parameter_struct rtc_cfg;
    rtc_datetime_t normalized = *datetime;

    if(!rtc_datetime_is_valid(&normalized)) {
        return;
    }

    normalized.day_of_week = rtc_compute_day_of_week(normalized.year, normalized.month, normalized.day);

    rtc_cfg.year = rtc_bin_to_bcd((uint8_t)(normalized.year % 100U));
    rtc_cfg.month = rtc_bin_to_bcd(normalized.month);
    rtc_cfg.date = rtc_bin_to_bcd(normalized.day);
    rtc_cfg.day_of_week = normalized.day_of_week;
    rtc_cfg.hour = rtc_bin_to_bcd(normalized.hour);
    rtc_cfg.minute = rtc_bin_to_bcd(normalized.minute);
    rtc_cfg.second = rtc_bin_to_bcd(normalized.second);
    rtc_cfg.factor_asyn = 127U;
    rtc_cfg.factor_syn = 255U;
    rtc_cfg.am_pm = RTC_AM;
    rtc_cfg.display_format = RTC_24HOUR;

    (void)rtc_init(&rtc_cfg);
}

void bsp_rtc_get_datetime(rtc_datetime_t *datetime)
{
    rtc_parameter_struct rtc_now;

    rtc_current_time_get(&rtc_now);

    datetime->year = (uint16_t)(2000U + rtc_bcd_to_bin(rtc_now.year));
    datetime->month = rtc_bcd_to_bin(rtc_now.month);
    datetime->day = rtc_bcd_to_bin(rtc_now.date);
    datetime->hour = rtc_bcd_to_bin(rtc_now.hour);
    datetime->minute = rtc_bcd_to_bin(rtc_now.minute);
    datetime->second = rtc_bcd_to_bin(rtc_now.second);
    datetime->day_of_week = rtc_now.day_of_week;
}

uint32_t bsp_rtc_get_epoch_seconds(void)
{
    rtc_datetime_t datetime;

    bsp_rtc_get_datetime(&datetime);
    return rtc_datetime_to_epoch(&datetime);
}

uint32_t bsp_rtc_backup_read(uint8_t index)
{
    if(index >= BSP_RTC_BACKUP_REG_COUNT) {
        return 0U;
    }

    return REG32(rtc_backup_reg_addr(index));
}

void bsp_rtc_backup_write(uint8_t index, uint32_t value)
{
    if(index >= BSP_RTC_BACKUP_REG_COUNT) {
        return;
    }

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    REG32(rtc_backup_reg_addr(index)) = value;
}
