#include "bsp/bsp_oled.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static char s_lines[BSP_OLED_LINE_COUNT][BSP_OLED_LINE_WIDTH + 1U];
static char s_shadow[BSP_OLED_LINE_COUNT][BSP_OLED_LINE_WIDTH + 1U];

static void oled_copy_line(char *dst, const char *src)
{
    size_t length = strlen(src);
    size_t index;

    if(length > BSP_OLED_LINE_WIDTH) {
        length = BSP_OLED_LINE_WIDTH;
    }

    for(index = 0U; index < BSP_OLED_LINE_WIDTH; ++index) {
        dst[index] = (index < length) ? src[index] : ' ';
    }
    dst[BSP_OLED_LINE_WIDTH] = '\0';
}

void bsp_oled_init(void)
{
    bsp_oled_clear();
    memcpy(s_shadow, s_lines, sizeof(s_lines));
}

void bsp_oled_clear(void)
{
    uint8_t line;

    for(line = 0U; line < BSP_OLED_LINE_COUNT; ++line) {
        memset(s_lines[line], ' ', BSP_OLED_LINE_WIDTH);
        s_lines[line][BSP_OLED_LINE_WIDTH] = '\0';
        s_shadow[line][0] = '\0';
    }
}

void bsp_oled_write_line(uint8_t line, const char *text)
{
    if(line >= BSP_OLED_LINE_COUNT) {
        return;
    }

    oled_copy_line(s_lines[line], text);
}

void bsp_oled_refresh(void)
{
    uint8_t line;
    bool changed = false;

    for(line = 0U; line < BSP_OLED_LINE_COUNT; ++line) {
        if(memcmp(s_lines[line], s_shadow[line], BSP_OLED_LINE_WIDTH + 1U) != 0) {
            changed = true;
            break;
        }
    }

    if(!changed) {
        return;
    }

    printf("\r\n[OLED]\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
           s_lines[0], s_lines[1], s_lines[2], s_lines[3]);

    memcpy(s_shadow, s_lines, sizeof(s_lines));
}
