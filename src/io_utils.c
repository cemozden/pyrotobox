#include <stdio.h>

//TODO: Add rom file check support for windows as well
#include <unistd.h>
#include "io_utils.h"

#define MIN_ROM_BIN_SIZE 0x10
#define MAX_ROM_BIN_SIZE 0x500000

const rom_read_result read_rom_bin(const char* rom_bin_path) {

    rom_read_result result = (rom_read_result) {
        .valid = false,
        .rom_bin = NULL,
        .size = 0
    };

    if (access(rom_bin_path, F_OK) != 0) {
        fprintf(stderr, "ERROR: The ROM file does not exist. Given Path: %s\n", rom_bin_path);
        return result;
    }

    FILE* rom_file = fopen(rom_bin_path, "rb");

    if (!rom_file) {
        fprintf(stderr, "ERROR: Unable to open the rom file. Given Path: %s\n", rom_bin_path);
        return result;
    }

    fseek(rom_file, 0, SEEK_END);
    const size_t rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    if (rom_size < MIN_ROM_BIN_SIZE) {
        fprintf(stderr, "ROM size cannot be less than 16 KiB. (Given ROM file size in bytes: %lu) Exiting...\n", rom_size);
        return result;
    }

    if (rom_size > MAX_ROM_BIN_SIZE) {
        fprintf(stderr, "ROM size cannot be more than 5 MiB. (Given ROM file size in bytes: %lu) Exiting...\n", rom_size);
        return result;
    }

    u8* rom_bin = malloc(sizeof(u8) * rom_size);

    if (!rom_bin) {
        fprintf(stderr, "ROM size cannot be more than 5 MiB. (Given ROM file size in bytes: %lu) Exiting...\n", rom_size);
        return result;
    }

    const size_t bytesRead = fread(rom_bin, sizeof(u8), rom_size, rom_file);

    if (bytesRead != rom_size) {
        fprintf(stderr, "Unable to load the ROM file into the memory. Exiting...\n");
        return result;
    }

    result.rom_bin = rom_bin;
    result.size = rom_size;
    result.valid = true;

    fclose(rom_file);

    return result;
}
