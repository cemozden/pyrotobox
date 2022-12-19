cmake_minimum_required(VERSION 3.16)

project(
        pyrotobox
        VERSION 0.1.0
        DESCRIPTION "A NES Emulator written in C"
        LANGUAGES C
    )

add_executable(pyrotobox src/main.c)

target_compile_features(pyrotobox PRIVATE c_std_99)
