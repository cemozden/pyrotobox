#ifndef UTILS_H
#define UTILS_H
#include "types.h"

const u16 read_little_endian_u16(u8* arr);
const u32 read_little_endian_u32(u8* arr);

void write_little_endian_u16(const u16 val, u8* arr);

#endif
