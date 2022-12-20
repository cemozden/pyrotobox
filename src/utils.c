#include "utils.h"

u16 read_little_endian_u16(u8* arr) {
    const u16 msb = arr[1] << 8;
    const u16 lsb = arr[1];
    const u16 result = msb | lsb;

    return result;
}

u32 read_little_endian_u32(u8* arr) {
    const u32 msb = arr[3] << 24;
    const u32 msb1 = arr[2] << 16;
    const u32 msb2 = arr[1] << 8;
    const u32 msb3 = arr[0];
    
    return msb | msb1 | msb2 | msb3;
}

void write_little_endian_u16(const u16 val, u8* arr) {
    const u8 msb = (val >> 8) & 0xFF;
    const u8 lsb = val & 0x00FF;

    arr[0] = lsb;
    arr[1] = msb;
}
