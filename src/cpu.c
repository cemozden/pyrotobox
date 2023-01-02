#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "utils.h"
#include <stdbool.h>

inline static u16 reset_vector(u8* cpu_mem);
inline static u16 irq_interrupt_vector(u8* cpu_mem);
static u8 read_u8(Cpu* cpu, u16 addr);

static void write_u8(Cpu* cpu, u16 addr, u8 val);

static operand_t get_operand(Cpu* cpu, AddrMode addr_mode);

// CPU flag ops
static void set_cpu_flag(Cpu* cpu, StatusFlag flag, bool set);
static inline bool get_cpu_flag(Cpu* cpu, StatusFlag flag);

// Stack ops
static bool push_stack(Cpu* cpu, u8 val);
static bool pop_stack(Cpu* cpu, u8* v_addr);

//6502 Instructions
static void adc(Cpu* cpu, operand_t* operand);
static void aand(Cpu* cpu, operand_t* operand);
static void asl(Cpu* cpu, operand_t* operand);
static void bcc(Cpu* cpu, operand_t* operand);
static void bcs(Cpu* cpu, operand_t* operand);
static void beq(Cpu* cpu, operand_t* operand);
static void brk(Cpu* cpu, operand_t* operand);


static Instruction MOS_6502_INSTRUCTION_SET[] = {
    {.mnemonic = "BRK", .addr_mode = IMPLIED, .cycles = 7, .exec = brk}
};

Cpu* build_cpu_from_mem(u8* cpu_mem) {
   Cpu* cpu = malloc(sizeof(Cpu));

   cpu->cpu_state = CPU_STOPPED;
   cpu->r_a  = cpu->r_x = cpu->r_y = 0;
   cpu->r_sp = STACK_SIZE;
   cpu->mem  = cpu_mem;
   cpu->r_pc = reset_vector(cpu_mem);
   cpu->r_sr = 0x04;

   return cpu;
}

size_t exec_instruction(Cpu* cpu) {
    const u8 opcode = read_u8(cpu, cpu->r_pc);
    const Instruction inst = MOS_6502_INSTRUCTION_SET[opcode];
    operand_t operand = get_operand(cpu, inst.addr_mode); 
    //TODO: Implement instruction printing in Assembly

    inst.exec(cpu, &operand);
    //TODO: If we return 0 cycles then stop NES (0 cycle can be returned in case of Stack overflow or underflow)
    return inst.cycles + operand.extra_cycles;
}

static inline u16 reset_vector(u8* cpu_mem) {
   return read_little_endian_u16(cpu_mem[0xFFFC], cpu_mem[0xFFFD]);
}

static inline u16 irq_interrupt_vector(u8* cpu_mem) {
   return read_little_endian_u16(cpu_mem[0xFFFE], cpu_mem[0xFFFF]);
}

static u8 read_u8(Cpu* cpu, u16 addr) {
    if (addr < 0x2000) {
        // 2KB RAM is mirrored towards 1FFF in memory map.
        const u16 masked_addr = addr & 0x07FF;
        return cpu->mem[masked_addr];
    }
    //PPU Registers
    else if (addr >= 0x2000 && addr < 0x4000) {
        //TODO: PPU register IO should be handled as soon as PPU implementation starts
        const u16 masked_addr = addr & 0x2007;
        printf("PPU is not implemented yet! CPU PC: %d", cpu->r_pc);
        exit(-4);
        return cpu->mem[masked_addr];
    }
    else return cpu->mem[addr];
}

static void write_u8(Cpu* cpu, u16 addr, u8 val) {
    if (addr < 0x2000) {
        // 2KB RAM is mirrored towards 1FFF in memory map.
        const u16 masked_addr = addr & 0x07FF;
        cpu->mem[masked_addr] = val;
    }
    //PPU Registers
    else if (addr >= 0x2000 && addr < 0x4000) {
        //TODO: PPU register IO should be handled as soon as PPU implementation starts
        //const u16 masked_addr = addr & 0x2007;
        printf("PPU Is not implemented yet! CPU PC: %d", cpu->r_pc);
        exit(-4);
    }
    else cpu->mem[addr] = val;
}

static operand_t get_operand(Cpu* cpu, AddrMode addr_mode) {
    operand_t operand = {.val = 0x00, .extra_cycles = 0x00, .addr_mode = addr_mode, .addr = 0x0000};
    
    switch (addr_mode) {
        case IMPLIED:
            cpu->r_pc++;
            break;
        case ACCUMULATOR:
            operand.val = cpu->r_a;
            cpu->r_pc++;
            break;
        case IMMEDIATE:
            operand.addr = cpu->r_pc + 1;
            operand.val = read_u8(cpu, cpu->r_pc + 1);
            cpu->r_pc += 2;
            break; 
        case ZERO_PAGE:
            operand.addr = read_u8(cpu, cpu->r_pc + 1);
            operand.val = read_u8(cpu, operand.addr);
            cpu->r_pc += 2;
            break; 
        case ZERO_PAGE_X: {
            const u16 zp_addr = read_u8(cpu, cpu->r_pc + 1);
            operand.addr = (zp_addr + cpu->r_x) & 0x00FF;
            operand.val = read_u8(cpu, operand.addr);
            cpu->r_pc += 2;
            break; 
       }
        case ZERO_PAGE_Y: {
            const u16 zp_addr = read_u8(cpu, cpu->r_pc + 1);
            operand.addr = (zp_addr + cpu->r_y) & 0x00FF;
            operand.val = read_u8(cpu, operand.addr);
            cpu->r_pc += 2;
            break; 
       }
        case RELATIVE: 
            // Relative offsetting will be handled inside of branch instructions
            break;
        case ABSOLUTE: {
            const u8 msb = read_u8(cpu, cpu->r_pc + 2);
            const u8 lsb = read_u8(cpu, cpu->r_pc + 1);
            const u16 addr = read_little_endian_u16(lsb, msb);
            operand.addr = addr;
            operand.val = read_u8(cpu, addr);
            cpu->r_pc += 3;
            break;
        }
        case ABSOLUTE_X: {
            const u8 msb = read_u8(cpu, cpu->r_pc + 2);
            const u8 lsb = read_u8(cpu, cpu->r_pc + 1);
            const u16 addr = read_little_endian_u16(lsb, msb) + cpu->r_x;
            operand.addr = addr & 0xFFFF;
            operand.val = read_u8(cpu, addr);
            //If the page changes then we should increase the instruction cycle by 1
            operand.extra_cycles = msb != ((addr >> 8) & 0xFF) ? 1 : 0;
            cpu->r_pc += 3;
            break;
        }
        case ABSOLUTE_Y: {
            const u8 msb = read_u8(cpu, cpu->r_pc + 2);
            const u8 lsb = read_u8(cpu, cpu->r_pc + 1);
            const u16 addr = read_little_endian_u16(lsb, msb) + cpu->r_y;
            operand.addr = addr & 0xFFFF;
            operand.val = read_u8(cpu, addr);
            //If the page changes then we should increase the instruction cycle by 1
            operand.extra_cycles = msb != ((addr >> 8) & 0xFF) ? 1 : 0;
            cpu->r_pc += 3;
            break;
        }
        case INDIRECT: {
            const u16 lsb_addr = read_little_endian_u16(read_u8(cpu, cpu->r_pc + 1), read_u8(cpu, cpu->r_pc + 2));
            const u8 lsb = read_u8(cpu, lsb_addr);
            const u8 msb = read_u8(cpu, lsb_addr + 1);
            const u16 addr = read_little_endian_u16(lsb, msb);
            operand.addr = addr;
            operand.val = read_u8(cpu, operand.addr);
            cpu->r_pc += 3;
            break;
        }
        case INDIRECT_X: {
            const u16 lsb_addr = (((u16)read_u8(cpu, cpu->r_pc + 1)) + cpu->r_x) & 0x00FF;
            const u8 lsb = read_u8(cpu, lsb_addr);
            const u8 msb = read_u8(cpu, lsb_addr + 1);
            const u16 addr = read_little_endian_u16(lsb, msb);
            operand.addr = addr;
            operand.val = read_u8(cpu, addr);
            cpu->r_pc += 2;
            break;
        }
        case INDIRECT_Y: {
            const u16 lsb_addr = (u16) read_u8(cpu, cpu->r_pc  + 1);
            const u8 lsb = read_u8(cpu, lsb_addr);
            const u8 msb = read_u8(cpu, lsb_addr + 1);
            u16 addr = (read_little_endian_u16(lsb, msb) + cpu->r_y) & 0xFFFF;
            operand.addr = addr;
            operand.val = read_u8(cpu, addr);
            operand.extra_cycles = msb != ((addr >> 8) & 0xFF) ? 1 : 0;
            cpu->r_pc += 2;
            break;
        }

    }
    return operand;
}

// 6502 INSTRUCTION IMPLEMENTATIONS
static void adc(Cpu* cpu, operand_t* operand) {
    const u16 sum = ((u16) cpu->r_a) + operand->val + ((u16) get_cpu_flag(cpu, CARRY_FLAG)); 

    set_cpu_flag(cpu, CARRY_FLAG, sum > 0x00FF);
    set_cpu_flag(cpu, ZERO_FLAG, sum == 0x0000);
    set_cpu_flag(cpu, NEGATIVE_FLAG, sum & 0x0080);
    set_cpu_flag(cpu, OVERFLOW_FLAG, (~((uint16_t) cpu->r_a ^ (uint16_t) operand->val) & ((uint16_t) cpu->r_a ^ (uint16_t) sum)) & 0x0080);
    
    cpu->r_a = sum & 0x00FF;
}

static void aand(Cpu* cpu, operand_t* operand) {
    const u8 result = cpu->r_a & operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);

    cpu->r_a = result;
}

static void asl(Cpu* cpu, operand_t* operand) {
    u8 old_val, new_val;

    if (operand->addr_mode == ACCUMULATOR) {
        old_val = cpu->r_a;
        cpu->r_a = new_val = cpu->r_a << 1;
    } else {
        old_val = operand->val;
        new_val = operand->val << 1;
        write_u8(cpu, operand->addr, new_val);
    }

    set_cpu_flag(cpu, CARRY_FLAG, old_val >> 7);
    set_cpu_flag(cpu, ZERO_FLAG, new_val == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, new_val & 0x80);
}

static void bcc(Cpu* cpu, operand_t* operand) {
    if (!get_cpu_flag(cpu, CARRY_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static void bcs(Cpu* cpu, operand_t* operand) {
    if (get_cpu_flag(cpu, CARRY_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static void beq(Cpu* cpu, operand_t* operand) {
    if (get_cpu_flag(cpu, ZERO_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static void brk(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
}

static void set_cpu_flag(Cpu* cpu, StatusFlag flag, bool set) {
    if (set) {
        cpu->r_sr |= flag;
    }
    else {
        cpu->r_sr &= ~flag;
    }
}

static inline bool get_cpu_flag(Cpu* cpu, StatusFlag flag) {
    return (cpu->r_sr & flag) > 0;
}

static bool push_stack(Cpu* cpu, u8 val) {
    if (cpu->r_sp == 0x00) {
        fprintf(stderr, "FATAL: Stack overflow occured at address 0x%x Exiting...", cpu->r_pc);
        return false; 
    }
    
    const u16 stack_addr = STACK_ADDR_OFFSET | cpu->r_sp;
    cpu->mem[stack_addr] = val;
    cpu->r_sp--;
    return true;
}

static bool pop_stack(Cpu* cpu, u8* v_addr) {
    if (cpu->r_sp == STACK_SIZE) {
        fprintf(stderr, "FATAL: Stack underflow occured at address 0x%x Exiting...", cpu->r_pc);
        return false; 
    }
    const u16 stack_addr = STACK_ADDR_OFFSET | cpu->r_sp;
    *v_addr = cpu->mem[stack_addr];
    cpu->r_sp++;
    return true;
}
