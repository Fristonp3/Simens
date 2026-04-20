#ifndef BL_MAIN_H
#define BL_MAIN_H

#include <stdbool.h>

void bl_main_init(void);
void bl_main_request_enter(void);
bool bl_main_requested(void);
void bl_main_process(void);
const char *bl_main_status_text(void);

#endif
