#include "bsp/bsp_gpio.h"

#include "board.h"
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

static const uint8_t s_led_count = 3U;
static const uint8_t s_key_count = 3U;
static key_tracker_t s_key_trackers[3];
static volatile bool s_wakeup_flag;

void bsp_gpio_init(void)
{
    uint8_t index;

    board_led_init();

    for (index = 0U; index < s_key_count; ++index) {
        board_key_init((board_key_t)index, BOARD_KEY_MODE_EXTI);
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

    for (index = 0U; index < s_key_count; ++index) {
        key_tracker_t *tracker = &s_key_trackers[index];
        bool raw_pressed = (board_key_state_get((board_key_t)index) == RESET);

        if (raw_pressed != tracker->sample_pressed) {
            tracker->sample_pressed = raw_pressed;
            tracker->last_change_ms = now_ms;
        }

        if (((now_ms - tracker->last_change_ms) >= GPIO_KEY_DEBOUNCE_MS) &&
            (tracker->stable_pressed != tracker->sample_pressed)) {
            tracker->stable_pressed = tracker->sample_pressed;
            if (tracker->stable_pressed) {
                tracker->pressed_ms = now_ms;
                tracker->long_reported = false;
                s_wakeup_flag = true;
            } else if (!tracker->long_reported) {
                tracker->pending_event = KEY_EVENT_SHORT;
            }
        }

        if (tracker->stable_pressed &&
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
    if (on) {
        board_led_on((uint8_t)led);
    } else {
        board_led_off((uint8_t)led);
    }
}

void bsp_gpio_toggle_led(bsp_led_t led)
{
    board_led_toggle((uint8_t)led);
}

bool bsp_gpio_take_wakeup_flag(void)
{
    bool value = s_wakeup_flag;
    s_wakeup_flag = false;
    return value;
}

void bsp_gpio_exti_irq_handler(uint32_t exti_line)
{
    (void)exti_line;
    exti_interrupt_flag_clear(exti_line);
    s_wakeup_flag = true;
}
