#include "bsp/bsp_power.h"

#include "gd32f4xx.h"

void bsp_power_init(void)
{
    /* Enable PMU clock for low-power operations */
    rcu_periph_clock_enable(RCU_PMU);

    /* Configure EXTI lines 0, 3, 13 as wakeup sources from sleep */
    /* This is done in board_key_init via EXTI configuration */

    /* Ensure backup domain is accessible for RTC/BKPSRAM */
    pmu_backup_write_enable();
}

void bsp_power_enter_sleep(void)
{
    /* Sleep mode with WFI - woken by any EXTI interrupt (keys, UART) */
    pmu_to_sleepmode(WFI_CMD);
}

void bsp_power_enter_deepsleep(void)
{
    /* Deep-sleep mode - woken by EXTI events from WAKE key (PA0) */
    pmu_to_deepsleepmode(PMU_LDO_NORMAL, PMU_LOWDRIVER_ENABLE, WFI_CMD);
}
