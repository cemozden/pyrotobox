#ifndef MAPPER_H
#define MAPPER_H
#include "types.h"
#include "nes.h"
#include <stdbool.h>

typedef struct MemMap {
    u8* cpu_mem_map;
    u8* ppu_mem_map;
} MemMap;

typedef struct mem_map_result { 
    bool valid;
    MemMap mem_map;
} mem_map_result;

mem_map_result generate_mem_map(const NesHeader* nes_header, const u8* rom_bin);

#endif
