cmake_minimum_required(VERSION 3.16)

project(
        pyrotobox
        VERSION 0.1.0
        DESCRIPTION "A NES Emulator written in C"
        LANGUAGES C
    )

add_executable(pyrotobox src/main.c src/types.h src/io_utils.h src/io_utils.c src/nes.h src/nes.c src/utils.h src/utils.c src/mapper.h src/mapper.c src/cpu.h src/cpu.c)

target_compile_features(pyrotobox PRIVATE c_std_99)

if(MSVC)
  target_compile_options(pyrotobox PRIVATE /W4 /WX)
else()
  target_compile_options(pyrotobox PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
