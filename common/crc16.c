#include "common/crc16.h"

uint16_t crc16_modbus(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    size_t index;
    uint8_t bit;

    for(index = 0; index < length; ++index) {
        crc ^= data[index];
        for(bit = 0U; bit < 8U; ++bit) {
            if((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}
