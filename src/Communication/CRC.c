// https://stackoverflow.com/questions/51731313/cross-platform-crc8-function-c-and-python-parity-check
#include "Main/Communication/CRC.h"

uint8_t crc8(uint8_t *addr, uint16_t len)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t inbyte = addr[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}
