#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "utils.h"

static u16 reset_vector(u8* cpu_mem);
static u8 read_u8(Cpu* cpu, u16 addr);

static void write_u8(Cpu* cpu, u16 addr, u8 val);

static operand_t get_operand(Cpu* cpu, AddrMode addr_mode);


static Instruction MOS_6502_INSTRUCTION_SET[] = {
    //{.mnemonic = "BRK", .addr_mode = IMPLIED, .run = brk}
};

Cpu* build_cpu_from_mem(u8* cpu_mem) {
   Cpu* cpu = malloc(sizeof(Cpu));

   cpu->cpu_state = CPU_STOPPED;
   cpu->r_a  = cpu->r_x = cpu->r_y = 0;
   cpu->r_sp = 0xFD;
   cpu->mem  = cpu_mem;
   cpu->r_pc = reset_vector(cpu_mem);
   cpu->r_sr = 0x04;

   return cpu;
}

size_t exec_instruction(Cpu* cpu) {
    const u8 opcode = read_u8(cpu, cpu->r_pc);
    const Instruction inst = MOS_6502_INSTRUCTION_SET[opcode];
    const operand_t operand = get_operand(cpu, inst.addr_mode); 
    //TODO: Implement instruction printing in Assembly

    return inst.run(cpu, &operand) + operand.extra_cycles;
}

static u16 reset_vector(u8* cpu_mem) {
   u8 reset_vec_arr[2];

   reset_vec_arr[0] = cpu_mem[0xFFFC];
   reset_vec_arr[1] = cpu_mem[0xFFFD];

   return read_little_endian_u16(reset_vec_arr);
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
    operand_t operand = {.val = 0x00, .extra_cycles = 0x00, .addr = 0x0000};
    
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
            u8 addr_arr[] = {lsb, msb};
            const u16 addr = read_little_endian_u16(addr_arr);
            operand.addr = addr;
            operand.val = read_u8(cpu, addr);
            cpu->r_pc += 3;
            break;
        }
        case ABSOLUTE_X: {
            const u8 msb = read_u8(cpu, cpu->r_pc + 2);
            const u8 lsb = read_u8(cpu, cpu->r_pc + 1);
            u8 addr_arr[] = {lsb, msb};
            const u16 addr = read_little_endian_u16(addr_arr) + cpu->r_x;
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
            u8 addr_arr[] = {lsb, msb};
            const u16 addr = read_little_endian_u16(addr_arr) + cpu->r_y;
            operand.addr = addr & 0xFFFF;
            operand.val = read_u8(cpu, addr);
            //If the page changes then we should increase the instruction cycle by 1
            operand.extra_cycles = msb != ((addr >> 8) & 0xFF) ? 1 : 0;
            cpu->r_pc += 3;
            break;
        }
        case INDIRECT: {
            u8 lsb_addr_arr[] = { read_u8(cpu, cpu->r_pc + 1), read_u8(cpu, cpu->r_pc + 2) };
            const u16 lsb_addr = read_little_endian_u16(lsb_addr_arr);
            const u8 lsb = read_u8(cpu, lsb_addr);
            const u8 msb = read_u8(cpu, lsb_addr + 1);
            u8 addr_arr[] = {lsb, msb};
            const u16 addr = read_little_endian_u16(addr_arr);
            operand.addr = addr;
            operand.val = read_u8(cpu, operand.addr);
            cpu->r_pc += 3;
            break;
        }
        case INDIRECT_X: {
            const u16 lsb_addr = (((u16)read_u8(cpu, cpu->r_pc + 1)) + cpu->r_x) & 0x00FF;
            const u8 lsb = read_u8(cpu, lsb_addr);
            const u8 msb = read_u8(cpu, lsb_addr + 1);
            u8 addr_arr[] = {lsb, msb};
            const u16 addr = read_little_endian_u16(addr_arr);
            operand.addr = addr;
            operand.val = read_u8(cpu, addr);
            cpu->r_pc += 2;
            break;
        }
        case INDIRECT_Y: {
            const u16 lsb_addr = (u16) read_u8(cpu, cpu->r_pc  + 1);
            const u8 lsb = read_u8(cpu, lsb_addr);
            const u8 msb = read_u8(cpu, lsb_addr + 1);
            u8 addr_arr[] = {lsb, msb};
            u16 addr = (read_little_endian_u16(addr_arr) + cpu->r_y) & 0xFFFF;
            operand.addr = addr;
            operand.val = read_u8(cpu, addr);
            operand.extra_cycles = msb != ((addr >> 8) & 0xFF) ? 1 : 0;
            cpu->r_pc += 2;
            break;
        }

    }

    return operand;
}
