#ifndef BL_JUMP_H
#define BL_JUMP_H

#include <stdint.h>

void bl_jump_to_application(uint32_t address);
uint32_t bl_jump_last_address(void);

#endif
