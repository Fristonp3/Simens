#include "bsp/bsp_power.h"

#include "gd32f4xx.h"

void bsp_power_init(void)
{
}

void bsp_power_enter_sleep(void)
{
    pmu_to_sleepmode(WFI_CMD);
}
