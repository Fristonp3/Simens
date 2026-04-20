#include "service/srv_display.h"

#include "bsp/bsp_oled.h"

void srv_display_init(void)
{
    bsp_oled_init();
}

void srv_display_present(const char *line0, const char *line1, const char *line2, const char *line3)
{
    bsp_oled_write_line(0U, line0);
    bsp_oled_write_line(1U, line1);
    bsp_oled_write_line(2U, line2);
    bsp_oled_write_line(3U, line3);
    bsp_oled_refresh();
}
