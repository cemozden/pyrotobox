#include "utils.h"

u16 read_little_endian_u16(u8 lsb, u8 msb) {
    return (msb << 8) | lsb;
}

u32 read_little_endian_u32(u8 msb4, u8 msb3, u8 msb2, u8 msb) {
    return (msb << 24) | (msb2 << 16) | (msb3 << 8) | msb4;
}

void write_little_endian_u16(const u16 val, u8* lsb, u8* msb) {
    *msb = (val >> 8) & 0xFF;
    *lsb = val & 0x00FF;
}
