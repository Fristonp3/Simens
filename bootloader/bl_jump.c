#include "bl_jump.h"

static uint32_t s_last_jump_address;

void bl_jump_to_application(uint32_t address)
{
    s_last_jump_address = address;
}

uint32_t bl_jump_last_address(void)
{
    return s_last_jump_address;
}
