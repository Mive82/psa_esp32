#include "header/van_helper.h"
#include "header/van_packet_parser.h"

uint16_t get_iden_from_bytes(uint8_t const byte1, uint8_t const byte2)
{
    uint16_t const iden = ((uint16_t)byte1 << 8) | (byte2 & 0xf0);
    return iden >> 4;
}

uint8_t dec_to_bcd(uint8_t input)
{
    if (input == 0xff)
    {
        return 0xff;
    }
    else if (input > 99)
    {
        return 0x99;
    }
    else
    {
        return ((input / 10 * 16) + (input % 10));
    }
}

uint8_t bcd_to_dec(uint8_t value)
{
    uint8_t ms_nibble = ((value & 0xF0) >> 4);
    uint8_t ls_nibble = (value & 0x0F);
    if (ms_nibble >= 10 || ls_nibble >= 10)
    {
        return value;
    }
    else
    {
        return ms_nibble * 10 + ls_nibble;
    }
}