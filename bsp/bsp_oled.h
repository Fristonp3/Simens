#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

#define BSP_OLED_LINE_COUNT  4U
#define BSP_OLED_LINE_WIDTH  32U

void bsp_oled_init(void);
void bsp_oled_clear(void);
void bsp_oled_write_line(uint8_t line, const char *text);
void bsp_oled_refresh(void);

#endif
