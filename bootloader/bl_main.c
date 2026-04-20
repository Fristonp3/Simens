#include "bl_main.h"

#include "bsp/bsp_uart.h"

static bool s_boot_requested;
static bool s_banner_printed;

void bl_main_init(void)
{
    s_boot_requested = false;
    s_banner_printed = false;
}

void bl_main_request_enter(void)
{
    s_boot_requested = true;
    s_banner_printed = false;
}

bool bl_main_requested(void)
{
    return s_boot_requested;
}

void bl_main_process(void)
{
    if(s_boot_requested && !s_banner_printed) {
        bsp_uart_send_string("\r\n[BOOT] Upgrade standby. Waiting for firmware stream.\r\n");
        s_banner_printed = true;
    }
}

const char *bl_main_status_text(void)
{
    return s_boot_requested ? "READY" : "IDLE";
}
