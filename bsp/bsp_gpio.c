#include "bsp/bsp_gpio.h"

#include "gd32f450i_eval.h"
#include "gd32f4xx_exti.h"

#define GPIO_KEY_DEBOUNCE_MS  30U
#define GPIO_KEY_LONG_MS      1200U

typedef struct {
    bool stable_pressed;
    bool sample_pressed;
    bool long_reported;
    uint32_t last_change_ms;
    uint32_t pressed_ms;
    key_event_t pending_event;
} key_tracker_t;

static const led_typedef_enum s_led_map[BSP_LED_COUNT] = {LED1, LED2, LED3};
static const key_typedef_enum s_key_map[BSP_KEY_COUNT] = {KEY_WAKEUP, KEY_TAMPER, KEY_USER};
static const exti_line_enum s_exti_map[BSP_KEY_COUNT] = {EXTI_0, EXTI_13, EXTI_14};
static key_tracker_t s_key_trackers[BSP_KEY_COUNT];
static volatile bool s_wakeup_flag;

void bsp_gpio_init(void)
{
    uint8_t index;

    for(index = 0U; index < BSP_LED_COUNT; ++index) {
        gd_eval_led_init(s_led_map[index]);
        gd_eval_led_off(s_led_map[index]);
    }

    for(index = 0U; index < BSP_KEY_COUNT; ++index) {
        gd_eval_key_init(s_key_map[index], KEY_MODE_EXTI);
        s_key_trackers[index].stable_pressed = false;
        s_key_trackers[index].sample_pressed = false;
        s_key_trackers[index].long_reported = false;
        s_key_trackers[index].last_change_ms = 0U;
        s_key_trackers[index].pressed_ms = 0U;
        s_key_trackers[index].pending_event = KEY_EVENT_NONE;
    }

    s_wakeup_flag = false;
}

void bsp_gpio_poll(uint32_t now_ms)
{
    uint8_t index;

    for(index = 0U; index < BSP_KEY_COUNT; ++index) {
        key_tracker_t *tracker = &s_key_trackers[index];
        bool raw_pressed = (gd_eval_key_state_get(s_key_map[index]) == RESET);

        if(raw_pressed != tracker->sample_pressed) {
            tracker->sample_pressed = raw_pressed;
            tracker->last_change_ms = now_ms;
        }

        if(((now_ms - tracker->last_change_ms) >= GPIO_KEY_DEBOUNCE_MS) &&
           (tracker->stable_pressed != tracker->sample_pressed)) {
            tracker->stable_pressed = tracker->sample_pressed;
            if(tracker->stable_pressed) {
                tracker->pressed_ms = now_ms;
                tracker->long_reported = false;
                s_wakeup_flag = true;
            } else if(!tracker->long_reported) {
                tracker->pending_event = KEY_EVENT_SHORT;
            }
        }

        if(tracker->stable_pressed &&
           (!tracker->long_reported) &&
           ((now_ms - tracker->pressed_ms) >= GPIO_KEY_LONG_MS)) {
            tracker->pending_event = KEY_EVENT_LONG;
            tracker->long_reported = true;
            s_wakeup_flag = true;
        }
    }
}

key_event_t bsp_gpio_take_key_event(bsp_key_t key)
{
    key_event_t event = s_key_trackers[key].pending_event;
    s_key_trackers[key].pending_event = KEY_EVENT_NONE;
    return event;
}

bool bsp_gpio_is_pressed(bsp_key_t key)
{
    return s_key_trackers[key].stable_pressed;
}

void bsp_gpio_set_led(bsp_led_t led, bool on)
{
    if(on) {
        gd_eval_led_on(s_led_map[led]);
    } else {
        gd_eval_led_off(s_led_map[led]);
    }
}

void bsp_gpio_toggle_led(bsp_led_t led)
{
    gd_eval_led_toggle(s_led_map[led]);
}

bool bsp_gpio_take_wakeup_flag(void)
{
    bool value = s_wakeup_flag;
    s_wakeup_flag = false;
    return value;
}

void bsp_gpio_exti_irq_handler(uint32_t exti_line)
{
    uint8_t index;

    for(index = 0U; index < BSP_KEY_COUNT; ++index) {
        if(s_exti_map[index] == exti_line) {
            exti_interrupt_flag_clear(s_exti_map[index]);
            s_wakeup_flag = true;
            break;
        }
    }
}
