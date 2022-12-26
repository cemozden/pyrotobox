#include "cpu.h"
#include "utils.h"
#include <stdlib.h>

static u16 reset_vector(u8* cpu_mem);

Cpu* build_cpu_from_mem(u8* cpu_mem) {
   Cpu* cpu = malloc(sizeof(Cpu));

   cpu->r_a  = cpu->r_x = cpu->r_y = 0;
   cpu->r_sp = 0xFD;
   cpu->mem  = cpu_mem;
   cpu->r_pc = reset_vector(cpu_mem);
   cpu->r_sr = 0x04;

   return cpu;
}

static u16 reset_vector(u8* cpu_mem) {
   u8 reset_vec_arr[2];

   reset_vec_arr[0] = cpu_mem[0xFFFC];
   reset_vec_arr[1] = cpu_mem[0xFFFD];

   return read_little_endian_u16(reset_vec_arr);
}
