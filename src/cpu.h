#ifndef CPU_H
#define CPU_H
#define STACK_SIZE 0xFF
#define STACK_ADDR_OFFSET 0x0100

#include "types.h"
#include <stdlib.h>

typedef enum CpuState {
    CPU_STOPPED,
    CPU_PAUSED,
    CPU_RUNNING
} CpuState;

typedef enum AddrMode {
    IMPLIED,
    ACCUMULATOR,
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    RELATIVE,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    INDIRECT,
    INDIRECT_X,
    INDIRECT_Y
} AddrMode;

typedef struct operand_t {
    u8 val;
    u8 extra_cycles;
    u8 bytes;
    AddrMode addr_mode;
    u16 addr;
} operand_t;

typedef enum StatusFlag {
    CARRY_FLAG = (1 << 0),
    ZERO_FLAG = (1 << 1),
    INTERRUPT_DISABLED_FLAG = (1 << 2),
    DECIMAL_FLAG = (1 << 3),
    BREAK_COMMAND = (1 << 4),
    UNUSED = (1 << 5),
    OVERFLOW_FLAG = (1 << 6),
    NEGATIVE_FLAG = (1 << 7),
} StatusFlag;

typedef struct Cpu {
    u8 r_x;
    u8 r_y;
    u8 r_a;
    u8 r_sp;
    u8 r_sr;
    u8* mem;
    CpuState cpu_state;
    u16 r_pc;
    u64 cycles;
    u64 instructions_performed;
} Cpu;

typedef struct Instruction {
    char mnemonic[3];
    AddrMode addr_mode;
    size_t cycles;
    void (*exec)(Cpu* cpu, operand_t* operand);
} Instruction;

Cpu* build_cpu_from_mem(u8* cpu_mem);
size_t exec_instruction(Cpu* cpu);

#endif
