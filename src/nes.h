#ifndef NES_H
#define NES_H

#include "types.h"
#include "cpu.h"
#include <stdbool.h>

typedef enum Mapper {
    NROM = 0
} Mapper;


typedef enum Mirroring {
    HORIZONTAL,
    VERTICAL
} Mirroring;

typedef struct NesHeader {
    bool prg_ram_available;
    u8 prg_rom_count;
    u8 chr_rom_count;
    Mapper mapper;
    Mirroring mirroring;
} NesHeader;

typedef struct Nes {
    NesHeader* nes_header;
    Cpu* cpu;
} Nes;

typedef struct build_nes_result_t {
    bool valid;
    Nes* nes;
} build_nes_result_t;

typedef struct build_nes_header_result_t {
    bool valid;
    NesHeader* nes_header;
} build_nes_header_result_t;

build_nes_result_t build_nes_from_rom_bin(u8** p_rom_bin);
void free_nes(Nes* nes);
void run_nes(Nes* nes);

#endif
