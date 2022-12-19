#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "io_utils.h"

#define INVALID_ARGUMENTS_ERROR_RETURN_CODE -1
#define READ_ROM_BIN_FAILED_ERROR_CODE -2

void print_help(void);

int main(int argc, char** argv) {

    if (argc < 2) { 
        print_help();
        return INVALID_ARGUMENTS_ERROR_RETURN_CODE;
    }

    const char* rom_bin_path = argv[1];

    const rom_read_result read_rom_bin_result = read_rom_bin(rom_bin_path);

    if (!read_rom_bin_result.valid) {
        return READ_ROM_BIN_FAILED_ERROR_CODE;
    }

    u8* rom_bin = read_rom_bin_result.rom_bin;

    const u8 first_byte = rom_bin[0];

    printf("First Byte: %x\n", first_byte);

    free(rom_bin);

    return 0;
}

void print_help(void) {
    printf("USAGE: pyrotobox <NES_ROM_FILE_PATH>\n");
}
