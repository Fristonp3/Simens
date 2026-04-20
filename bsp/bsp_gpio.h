#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "common/project_types.h"

typedef enum {
    BSP_LED_RUN = 0,
    BSP_LED_RECORD,
    BSP_LED_STATUS,
    BSP_LED_COUNT
} bsp_led_t;

typedef enum {
    BSP_KEY_WAKE = 0,
    BSP_KEY_MODE,
    BSP_KEY_NEXT,
    BSP_KEY_COUNT
} bsp_key_t;

void bsp_gpio_init(void);
void bsp_gpio_poll(uint32_t now_ms);
key_event_t bsp_gpio_take_key_event(bsp_key_t key);
bool bsp_gpio_is_pressed(bsp_key_t key);
void bsp_gpio_set_led(bsp_led_t led, bool on);
void bsp_gpio_toggle_led(bsp_led_t led);
bool bsp_gpio_take_wakeup_flag(void);
void bsp_gpio_exti_irq_handler(uint32_t exti_line);

#endif
