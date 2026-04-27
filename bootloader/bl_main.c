#include "bl_main.h"

#include "bl_flash.h"
#include "bl_jump.h"
#include "bl_uart.h"
#include "bsp/bsp_uart.h"

#define BL_APP_START_ADDR    0x08008000U

static bool s_boot_requested;
static bool s_banner_printed;
static bl_channel_t s_locked;

void bl_main_init(void)
{
    s_boot_requested = false;
    s_banner_printed = false;
    s_locked = BL_CHAN_NONE;
}

void bl_main_request_enter(void)
{
    s_boot_requested = true;
    s_banner_printed = false;
    s_locked = BL_CHAN_NONE;
    bl_uart_reset();
    bl_flash_reset();
}

bool bl_main_requested(void)
{
    return s_boot_requested;
}

void bl_main_process(void)
{
    if (!s_boot_requested) {
        return;
    }

    if (!s_banner_printed) {
        bsp_uart_send_string("\r\n[BOOT] Upgrade standby. UART0+UART2 ready.\r\n");
        bsp_uart_bt_send_string("\r\n[BOOT] Upgrade standby. BT channel ready.\r\n");
        s_banner_printed = true;
    }

    bl_uart_poll();

    if (bl_uart_frame_ready()) {
        bl_channel_t ch = bl_uart_locked_channel();
        if (s_locked == BL_CHAN_NONE && ch != BL_CHAN_NONE) {
            s_locked = ch;
            if (s_locked == BL_CHAN_BT) {
                bsp_uart_bt_send_string("[BOOT] BT channel locked. Receiving firmware...\r\n");
            } else {
                bsp_uart_send_string("[BOOT] UART0 channel locked. Receiving firmware...\r\n");
            }
        }
        bl_uart_process_frame();
    }
}

bool bl_main_should_jump(void)
{
    return bl_flash_is_complete();
}

void bl_main_do_jump(void)
{
    const char *msg = "\r\n[BOOT] Firmware complete. Jumping to application...\r\n";

    if (s_locked == BL_CHAN_BT) {
        bsp_uart_bt_send_string(msg);
    } else {
        bsp_uart_send_string(msg);
    }
    bl_jump_to_application(BL_APP_START_ADDR);
}

const char *bl_main_status_text(void)
{
    if (!s_boot_requested) {
        return "IDLE";
    }
    if (s_locked == BL_CHAN_BT) {
        return "BT_BOOT";
    }
    return "READY";
}
