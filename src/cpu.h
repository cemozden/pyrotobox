#ifndef CPU_H
#define CPU_H
#include "types.h"

typedef struct Cpu {
    u8 r_x;
    u8 r_y;
    u8 r_a;
    u8 r_sp;
    u8 r_sr;
    u8* mem;
    u16 r_pc;
} Cpu;

Cpu* build_cpu_from_mem(u8* cpu_mem);

#endif
