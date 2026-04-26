/*!
    \file    gd32f4xx_it.h
    \brief   the header file of the ISR

    \version 2026-02-05, V3.3.3, firmware for GD32F4xx
*/

#ifndef GD32F4XX_IT_H
#define GD32F4XX_IT_H

#include "gd32f4xx.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void EXTI0_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI10_15_IRQHandler(void);
void USART0_IRQHandler(void);

#endif /* GD32F4XX_IT_H */
