#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "cpu.h"
#include "nes.h"
#include "mapper.h"
#include "utils.h"

#define INES_HEADER_SIGNATURE 0x1A53454E

static build_nes_header_result_t build_nes_header_from_rom_bin(const u8* rom_bin) {
    build_nes_header_result_t result = (build_nes_header_result_t) {
        .nes_header = NULL,
        .valid = false
    };
    const size_t sign_size_in_bytes = 0x04;
    u8 sign_arr[sign_size_in_bytes];
    memcpy(sign_arr, rom_bin, sign_size_in_bytes);

    const u32 sign = read_little_endian_u32(rom_bin[0], rom_bin[1], rom_bin[2], rom_bin[3]);

    if (sign != INES_HEADER_SIGNATURE) {
        fprintf(stderr, "Invalid iNES Header signature! Given Signature: 0x%x\n", sign);
        return result;
    }

    NesHeader* nes_header = malloc(sizeof(NesHeader));

    nes_header->prg_rom_count = rom_bin[4];
    nes_header->chr_rom_count = rom_bin[5];
    nes_header->mirroring = (rom_bin[6] & 0x1) == 1 ? VERTICAL : HORIZONTAL;
    nes_header->prg_ram_available = (rom_bin[6] & 0x10) > 0;

    const u8 mapper_code = rom_bin[6] >> 4;

    switch (mapper_code) {
         case 0:
                nes_header->mapper = NROM;
                break;
        default: 
                 fprintf(stderr, "Invalid iNES Mapper code: %d", mapper_code);
                 // Free NES Header as the header is invalid.
                 free(nes_header);
                 return result;
    }

    result.nes_header = nes_header;
    result.valid = true;

    return result;
}

build_nes_result_t build_nes_from_rom_bin(u8** p_rom_bin) {
    u8* rom_bin = *p_rom_bin;
    build_nes_result_t result = (build_nes_result_t) {.nes = NULL, .valid = false};
    const build_nes_header_result_t nes_header_result = build_nes_header_from_rom_bin(rom_bin);

    if(!nes_header_result.valid) {
        free(rom_bin);
        //Update the pointer address to NULL after cleaning up.
        *p_rom_bin = NULL;
        return result;
    }

    Nes* nes = malloc(sizeof(Nes));

    nes->nes_header = nes_header_result.nes_header;
    mem_map_result mem_map_result = generate_mem_map(nes->nes_header, rom_bin);

    if (!mem_map_result.valid) {
        // Free NES if we can't create mem map
        free(nes);
        free(rom_bin);
        *p_rom_bin = NULL;

        return result;
    }

    u8* cpu_mem = mem_map_result.mem_map.cpu_mem_map;
    Cpu* cpu = build_cpu_from_mem(cpu_mem);
    nes->cpu = cpu;

    result.nes = nes;
    result.valid = true;

    free(rom_bin);
    *p_rom_bin = NULL;


    return result;
}

void run_nes(Nes* nes) {
    Cpu* cpu = nes->cpu;
    cpu->cpu_state = CPU_RUNNING;

    //FIXME: Implement the infinite loop speed according to 2A03 CPU clock cycle.
    while (cpu->cpu_state == CPU_RUNNING) {
       const size_t cycles = exec_instruction(cpu);

       if (cycles == 0) {
            printf("Error occured while CPU is processing instructions at address $%x\n", cpu->r_pc);
            break;
       }

       cpu->cycles += cycles;
       cpu->instructions_performed++;
    }
}

void free_nes(Nes* nes) {
    free(nes->nes_header);
    free(nes->cpu->mem);
    free(nes->cpu);
    free(nes);
}
