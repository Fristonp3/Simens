/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines

    \version 2026-02-05, V3.3.3, firmware for GD32F4xx
*/

#include "gd32f4xx_it.h"
#include "board.h"
#include "bsp/bsp_gpio.h"
#include "bsp/bsp_uart.h"
#include "systick.h"

void NMI_Handler(void)
{
    while (1) {
    }
}

void HardFault_Handler(void)
{
    while (1) {
    }
}

void MemManage_Handler(void)
{
    while (1) {
    }
}

void BusFault_Handler(void)
{
    while (1) {
    }
}

void UsageFault_Handler(void)
{
    while (1) {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    systick_irq_handler();
}

/* EXTI line 0 - WAKE key (PA0) */
void EXTI0_IRQHandler(void)
{
    if (SET == exti_interrupt_flag_get(EXTI_0)) {
        bsp_gpio_exti_irq_handler(EXTI_0);
    }
}

/* EXTI line 3 - NEXT key (PE3) */
void EXTI3_IRQHandler(void)
{
    if (SET == exti_interrupt_flag_get(EXTI_3)) {
        bsp_gpio_exti_irq_handler(EXTI_3);
    }
}

/* EXTI lines 10-15 - MODE key (PC13) */
void EXTI10_15_IRQHandler(void)
{
    if (SET == exti_interrupt_flag_get(EXTI_13)) {
        bsp_gpio_exti_irq_handler(EXTI_13);
    }
}

/* USART0 interrupt handler */
void USART0_IRQHandler(void)
{
    bsp_uart_irq_handler();
}
