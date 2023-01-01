#ifndef UTILS_H
#define UTILS_H
#include "types.h"

u16 read_little_endian_u16(u8 lsb, u8 msb);
u32 read_little_endian_u32(u8 msb4, u8 msb3, u8 msb2, u8 msb);

void write_little_endian_u16(const u16 val, u8* lsb, u8* msb);

#endif
