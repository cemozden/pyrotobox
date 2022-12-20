#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "io_utils.h"
#include "nes.h"

//Versioning
#define PYROTOBOX_MAJOR_VERSION 0
#define PYROTOBOX_MINOR_VERSION 1
#define PYROTOBOX_PATCH_VERSION 0

#define INVALID_ARGUMENTS_ERROR_RETURN_CODE -1
#define READ_ROM_BIN_FAILED_ERROR_RETURN_CODE -2
#define NES_BUILD_FAILED_ERROR_RETURN_CODE -3

void print_help(void);

int main(int argc, char** argv) {
    if (argc < 2) { 
        print_help();
        return INVALID_ARGUMENTS_ERROR_RETURN_CODE;
    }

    const char* rom_bin_path = argv[1];
    const rom_read_result read_rom_bin_result = read_rom_bin(rom_bin_path);

    if (!read_rom_bin_result.valid) {
        return READ_ROM_BIN_FAILED_ERROR_RETURN_CODE;
    }

    u8* rom_bin = read_rom_bin_result.rom_bin;
    const build_nes_result_t build_nes_result = build_nes_from_rom_bin(&rom_bin);

    if (!build_nes_result.valid) {
        return NES_BUILD_FAILED_ERROR_RETURN_CODE;
    }
    
    Nes* nes = build_nes_result.nes;

    printf("\n> pyrotobox v%d.%d.%d, A NES Emulator\n\n", PYROTOBOX_MAJOR_VERSION, PYROTOBOX_MINOR_VERSION, PYROTOBOX_PATCH_VERSION);
    printf("ROM Path: %s\n", rom_bin_path);

    const char mirr_horizontal_str[] = "Horizontal";
    const char mirr_vertical_str[] = "Vertical";
    
    printf("PRG ROM Size: %d, CHR ROM Size: %d, Mirroring: %s\n",
            nes->nes_header->prg_rom_count,
            nes->nes_header->chr_rom_count,
            nes->nes_header->mirroring == HORIZONTAL ? mirr_horizontal_str : mirr_vertical_str
          );

    free_nes(nes);

    return 0;
}

void print_help(void) {
    printf("USAGE: pyrotobox <NES_ROM_FILE_PATH>\n");
}
