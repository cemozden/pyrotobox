#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdbool.h>
#include <stdlib.h>
#include "types.h"

typedef struct rom_read_result {
    bool valid;
    u8* rom_bin;
    size_t size;
} rom_read_result;

rom_read_result read_rom_bin(const char* rom_bin_path);

#endif
