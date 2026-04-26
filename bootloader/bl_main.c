#include "bl_main.h"

#include "bl_flash.h"
#include "bl_jump.h"
#include "bl_uart.h"
#include "bsp/bsp_uart.h"

#define BL_APP_START_ADDR    0x08008000U
#define BL_TIMEOUT_MS        5000U

static bool s_boot_requested;
static bool s_banner_printed;
static uint32_t s_entry_ms;

void bl_main_init(void)
{
    s_boot_requested = false;
    s_banner_printed = false;
    s_entry_ms = 0U;
}

void bl_main_request_enter(void)
{
    s_boot_requested = true;
    s_banner_printed = false;
    s_entry_ms = 0U;
    bl_uart_reset();
    bl_flash_reset();
}

bool bl_main_requested(void)
{
    return s_boot_requested;
}

void bl_main_process(void)
{
    uint8_t byte;

    if (!s_boot_requested) {
        return;
    }

    if (!s_banner_printed) {
        bsp_uart_send_string("\r\n[BOOT] Upgrade standby. Waiting for firmware stream.\r\n");
        s_banner_printed = true;
        s_entry_ms = 0U;
    }

    /* Process incoming firmware data from UART */
    while (s_boot_requested && bsp_uart_read_byte(&byte)) {
        bl_uart_feed_byte(byte);

        if (bl_uart_frame_complete()) {
            bl_uart_process_frame();
        }
    }
}

bool bl_main_should_jump(void)
{
    return bl_flash_is_complete();
}

void bl_main_do_jump(void)
{
    bsp_uart_send_string("\r\n[BOOT] Firmware complete. Jumping to application...\r\n");
    bl_jump_to_application(BL_APP_START_ADDR);
}

const char *bl_main_status_text(void)
{
    if (!s_boot_requested) {
        return "IDLE";
    }
    return "READY";
}
