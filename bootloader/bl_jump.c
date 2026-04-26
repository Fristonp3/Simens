#include "bl_jump.h"

#include "gd32f4xx.h"

static uint32_t s_last_jump_address;

void bl_jump_to_application(uint32_t address)
{
    uint32_t app_stack;
    uint32_t app_reset_handler;

    s_last_jump_address = address;

    /* Validate the vector table (SP must be in RAM, reset handler in flash) */
    app_stack = *(volatile uint32_t *)address;
    app_reset_handler = *(volatile uint32_t *)(address + 4U);

    if ((app_stack < 0x20000000U) || (app_stack > 0x20030000U)) {
        return;  /* Invalid stack pointer */
    }
    if ((app_reset_handler < 0x08000000U) || (app_reset_handler > 0x0807FFFFU)) {
        return;  /* Invalid reset handler */
    }

    /* Disable all interrupts */
    __disable_irq();

    /* Disable SysTick */
    SysTick->CTRL = 0U;

    /* Reset all peripherals to default state */
    /* Set vector table offset to application */
    SCB->VTOR = address;

    /* Set the stack pointer */
    __set_MSP(app_stack);

    /* Jump to application reset handler */
    ((void (*)(void))app_reset_handler)();

    /* Should never reach here */
    while (1) {
    }
}

uint32_t bl_jump_last_address(void)
{
    return s_last_jump_address;
}
