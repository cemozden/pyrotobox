#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

#include "mapper.h"
#include "nes.h"

#define CPU_MEM_MAP_SIZE 0x10000
#define PRG_ROM_SIZE_PER_UNIT 0x4000
#define CHR_ROM_SIZE 0x2000

static MemMap get_nrom_mem_map(const NesHeader* nes_header, const u8* rom_bin);

mem_map_result generate_mem_map(const NesHeader* nes_header, const u8* rom_bin) {
    mem_map_result result = (mem_map_result) { .valid = false };
    switch (nes_header->mapper) {
        case NROM: 
            result.mem_map = get_nrom_mem_map(nes_header, rom_bin);
            break;
        default:
            fprintf(stderr, "Cannot get ROM due to invalid mapper. Mapper code: %d", nes_header->mapper);
            return result;
    }

    result.valid = true;
    return result;
}

static MemMap get_nrom_mem_map(const NesHeader* nes_header, const u8* rom_bin) {
    MemMap mem_map = (MemMap) {.cpu_mem_map = NULL, .ppu_mem_map = NULL};
    u8* cpu_mem_map = calloc(CPU_MEM_MAP_SIZE, sizeof(u8));

    int i;
    for (i = 0 ; i < CPU_MEM_MAP_SIZE; i++) cpu_mem_map[i] = 0xFF;

    //TODO: Support battery-packed PRG RAMs?

    //There will always be at most 2 banks for PRG in NROM typed cartridges
    if (nes_header->prg_rom_count > 1) {
        memcpy(&cpu_mem_map[0x8000], &rom_bin[0x10], PRG_ROM_SIZE_PER_UNIT);
        memcpy(&cpu_mem_map[0xC000], &rom_bin[0x4010], PRG_ROM_SIZE_PER_UNIT);
    } else {
        memcpy(&cpu_mem_map[0x8000], &rom_bin[0x10], PRG_ROM_SIZE_PER_UNIT);
        //Mirror the single PRG ROM
        memcpy(&cpu_mem_map[0xC000], &rom_bin[0x10], PRG_ROM_SIZE_PER_UNIT);
    }

    mem_map.cpu_mem_map = cpu_mem_map;
    return mem_map;
}
