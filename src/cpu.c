#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "cpu.h"
#include "utils.h"

inline static u16 reset_vector(u8* cpu_mem);
inline static u16 irq_interrupt_vector(u8* cpu_mem);
static u8 read_u8(Cpu* cpu, u16 addr);

static void write_u8(Cpu* cpu, u16 addr, u8 val);
static void print_inst(const Cpu* cpu, const operand_t* operand, const Instruction* inst);

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
static void bit(Cpu* cpu, operand_t* operand);
static void bmi(Cpu* cpu, operand_t* operand);
static void bne(Cpu* cpu, operand_t* operand);
static void bpl(Cpu* cpu, operand_t* operand);
static void brk(Cpu* cpu, operand_t* operand);
static void bvc(Cpu* cpu, operand_t* operand);
static void bvs(Cpu* cpu, operand_t* operand);
static inline void clc(Cpu* cpu, operand_t* operand);
static inline void cld(Cpu* cpu, operand_t* operand);
static inline void cli(Cpu* cpu, operand_t* operand);
static inline void clv(Cpu* cpu, operand_t* operand);
static void cmp(Cpu* cpu, operand_t* operand);
static void cpx(Cpu* cpu, operand_t* operand);
static void cpy(Cpu* cpu, operand_t* operand);
static void dec(Cpu* cpu, operand_t* operand);
static void dex(Cpu* cpu, operand_t* operand);
static void dey(Cpu* cpu, operand_t* operand);
static void eor(Cpu* cpu, operand_t* operand);
static void inc(Cpu* cpu, operand_t* operand);
static void inx(Cpu* cpu, operand_t* operand);
static void iny(Cpu* cpu, operand_t* operand);
static void jmp(Cpu* cpu, operand_t* operand);
static void jsr(Cpu* cpu, operand_t* operand);
static void lda(Cpu* cpu, operand_t* operand);
static void ldx(Cpu* cpu, operand_t* operand);
static void ldy(Cpu* cpu, operand_t* operand);
static void lsr(Cpu* cpu, operand_t* operand);
static inline void nop(Cpu* cpu, operand_t* operand);
static void ora(Cpu* cpu, operand_t* operand);
static inline void pha(Cpu* cpu, operand_t* operand);
static inline void php(Cpu* cpu, operand_t* operand);
static void pla(Cpu* cpu, operand_t* operand);
static inline void plp(Cpu* cpu, operand_t* operand);
static void rol(Cpu* cpu, operand_t* operand);
static void ror(Cpu* cpu, operand_t* operand);
static void rti(Cpu* cpu, operand_t* operand);
static void rts(Cpu* cpu, operand_t* operand);
static void sbc(Cpu* cpu, operand_t* operand);
static inline void sec(Cpu* cpu, operand_t* operand);
static inline void sed(Cpu* cpu, operand_t* operand);
static inline void sei(Cpu* cpu, operand_t* operand);
static inline void sta(Cpu* cpu, operand_t* operand);
static inline void stx(Cpu* cpu, operand_t* operand);
static inline void sty(Cpu* cpu, operand_t* operand);
static void tax(Cpu* cpu, operand_t* operand);
static void tay(Cpu* cpu, operand_t* operand);
static void tsx(Cpu* cpu, operand_t* operand);
static void txa(Cpu* cpu, operand_t* operand);
static inline void txs(Cpu* cpu, operand_t* operand);
static void tya(Cpu* cpu, operand_t* operand);


static Instruction MOS_6502_INSTRUCTION_SET[] = {
    {.mnemonic = "BRK", .addr_mode = IMPLIED, .cycles = 7, .exec = brk},
    {.mnemonic = "ORA", .addr_mode = INDIRECT_X, .cycles = 6, .exec = ora},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ORA", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = ora},
    {.mnemonic = "ASL", .addr_mode = ZERO_PAGE, .cycles = 5, .exec = asl},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "php", .addr_mode = IMPLIED, .cycles = 3, .exec = php},
    {.mnemonic = "ORA", .addr_mode = IMMEDIATE, .cycles = 2, .exec = ora},
    {.mnemonic = "ASL", .addr_mode = ACCUMULATOR, .cycles = 2, .exec = asl},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ORA", .addr_mode = ABSOLUTE, .cycles = 4, .exec = ora},
    {.mnemonic = "ASL", .addr_mode = ABSOLUTE, .cycles = 6, .exec = asl},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BPL", .addr_mode = IMPLIED, .cycles = 2, .exec = bpl},
    {.mnemonic = "ORA", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = ora},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ORA", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = ora},
    {.mnemonic = "ASL", .addr_mode = ZERO_PAGE_X, .cycles = 6, .exec = asl},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CLC", .addr_mode = IMPLIED, .cycles = 2, .exec = clc},
    {.mnemonic = "ORA", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = ora},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ORA", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = ora},
    {.mnemonic = "ASL", .addr_mode = ABSOLUTE_X, .cycles = 7, .exec = asl},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "JSR", .addr_mode = ABSOLUTE, .cycles = 6, .exec = jsr},
    {.mnemonic = "AND", .addr_mode = INDIRECT_X, .cycles = 6, .exec = aand},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BIT", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = bit},
    {.mnemonic = "AND", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = aand},
    {.mnemonic = "ROL", .addr_mode = ZERO_PAGE, .cycles = 5, .exec = rol},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "PLP", .addr_mode = IMPLIED, .cycles = 4, .exec = plp},
    {.mnemonic = "AND", .addr_mode = IMMEDIATE, .cycles = 2, .exec = aand},
    {.mnemonic = "ROL", .addr_mode = ACCUMULATOR, .cycles = 2, .exec = rol},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BIT", .addr_mode = ABSOLUTE, .cycles = 4, .exec = bit},
    {.mnemonic = "AND", .addr_mode = ABSOLUTE, .cycles = 4, .exec = aand},
    {.mnemonic = "ROL", .addr_mode = ABSOLUTE, .cycles = 6, .exec = rol},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BMI", .addr_mode = IMPLIED, .cycles = 2, .exec = bmi},
    {.mnemonic = "AND", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = aand},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "AND", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = aand},
    {.mnemonic = "ROL", .addr_mode = ZERO_PAGE_X, .cycles = 6, .exec = rol},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "SEC", .addr_mode = IMPLIED, .cycles = 2, .exec = sec},
    {.mnemonic = "AND", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = aand},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "AND", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = aand},
    {.mnemonic = "ROL", .addr_mode = ABSOLUTE_X, .cycles = 7, .exec = rol},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "RTI", .addr_mode = IMPLIED, .cycles = 6, .exec = rti},
    {.mnemonic = "EOR", .addr_mode = INDIRECT_X, .cycles = 6, .exec = eor},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "EOR", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = eor},
    {.mnemonic = "LSR", .addr_mode = ZERO_PAGE, .cycles = 5, .exec = lsr},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "PHA", .addr_mode = IMPLIED, .cycles = 3, .exec = pha},
    {.mnemonic = "EOR", .addr_mode = IMMEDIATE, .cycles = 2, .exec = eor},
    {.mnemonic = "LSR", .addr_mode = ACCUMULATOR, .cycles = 2, .exec = lsr},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "JMP", .addr_mode = ABSOLUTE, .cycles = 3, .exec = jmp},
    {.mnemonic = "EOR", .addr_mode = ABSOLUTE, .cycles = 4, .exec = eor},
    {.mnemonic = "LSR", .addr_mode = ABSOLUTE, .cycles = 6, .exec = lsr},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BVC", .addr_mode = IMPLIED, .cycles = 2, .exec = bvc},
    {.mnemonic = "EOR", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = eor},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "EOR", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = eor},
    {.mnemonic = "LSR", .addr_mode = ZERO_PAGE_X, .cycles = 6, .exec = lsr},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CLI", .addr_mode = IMPLIED, .cycles = 2, .exec = cli},
    {.mnemonic = "EOR", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = eor},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "EOR", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = eor},
    {.mnemonic = "LSR", .addr_mode = ABSOLUTE_X, .cycles = 7, .exec = lsr},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "RTS", .addr_mode = IMPLIED, .cycles = 6, .exec = rts},
    {.mnemonic = "ADC", .addr_mode = INDIRECT_X, .cycles = 6, .exec = adc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ADC", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = adc},
    {.mnemonic = "ROR", .addr_mode = ZERO_PAGE, .cycles = 5, .exec = ror},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "PLA", .addr_mode = IMPLIED, .cycles = 4, .exec = pla},
    {.mnemonic = "ADC", .addr_mode = IMMEDIATE, .cycles = 2, .exec = adc},
    {.mnemonic = "ROR", .addr_mode = ACCUMULATOR, .cycles = 2, .exec = ror},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "JMP", .addr_mode = INDIRECT, .cycles = 5, .exec = jmp},
    {.mnemonic = "ADC", .addr_mode = ABSOLUTE, .cycles = 4, .exec = adc},
    {.mnemonic = "ROR", .addr_mode = ABSOLUTE, .cycles = 6, .exec = ror},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BVS", .addr_mode = IMPLIED, .cycles = 2, .exec = bvs},
    {.mnemonic = "ADC", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = adc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ADC", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = adc},
    {.mnemonic = "ROR", .addr_mode = ZERO_PAGE_X, .cycles = 6, .exec = ror},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "SEI", .addr_mode = IMPLIED, .cycles = 2, .exec = sei},
    {.mnemonic = "ADC", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = adc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "ADC", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = adc},
    {.mnemonic = "ROR", .addr_mode = ABSOLUTE_X, .cycles = 7, .exec = ror},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "STA", .addr_mode = INDIRECT_X, .cycles = 6, .exec = sta},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "STY", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = sty},
    {.mnemonic = "STA", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = sta},
    {.mnemonic = "STX", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = stx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "DEY", .addr_mode = IMPLIED, .cycles = 2, .exec = dey},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "TXA", .addr_mode = IMPLIED, .cycles = 2, .exec = txa},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "STY", .addr_mode = ABSOLUTE, .cycles = 4, .exec = sty},
    {.mnemonic = "STA", .addr_mode = ABSOLUTE, .cycles = 4, .exec = sta},
    {.mnemonic = "STX", .addr_mode = ABSOLUTE, .cycles = 4, .exec = stx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BCC", .addr_mode = IMPLIED, .cycles = 2, .exec = bcc},
    {.mnemonic = "STA", .addr_mode = INDIRECT_Y, .cycles = 6, .exec = sta},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "STY", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = sty},
    {.mnemonic = "STA", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = sta},
    {.mnemonic = "STX", .addr_mode = ZERO_PAGE_Y, .cycles = 4, .exec = stx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "TYA", .addr_mode = IMPLIED, .cycles = 2, .exec = tya},
    {.mnemonic = "STA", .addr_mode = ABSOLUTE_Y, .cycles = 5, .exec = sta},
    {.mnemonic = "TXS", .addr_mode = IMPLIED, .cycles = 2, .exec = txs},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "STA", .addr_mode = ABSOLUTE_X, .cycles = 5, .exec = sta},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "LDY", .addr_mode = IMMEDIATE, .cycles = 2, .exec = ldy},
    {.mnemonic = "LDA", .addr_mode = INDIRECT_X, .cycles = 6, .exec = lda},
    {.mnemonic = "LDX", .addr_mode = IMMEDIATE, .cycles = 2, .exec = ldx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "LDY", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = ldy},
    {.mnemonic = "LDA", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = lda},
    {.mnemonic = "LDX", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = ldx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "TAY", .addr_mode = IMPLIED, .cycles = 2, .exec = tay},
    {.mnemonic = "LDA", .addr_mode = IMMEDIATE, .cycles = 2, .exec = lda},
    {.mnemonic = "TAX", .addr_mode = IMPLIED, .cycles = 2, .exec = tax},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "LDY", .addr_mode = ABSOLUTE, .cycles = 4, .exec = ldy},
    {.mnemonic = "LDA", .addr_mode = ABSOLUTE, .cycles = 4, .exec = lda},
    {.mnemonic = "LDX", .addr_mode = ABSOLUTE, .cycles = 4, .exec = ldx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BCS", .addr_mode = IMPLIED, .cycles = 2, .exec = bcs},
    {.mnemonic = "LDA", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = lda},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "LDY", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = ldy},
    {.mnemonic = "LDA", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = lda},
    {.mnemonic = "LDX", .addr_mode = ZERO_PAGE_Y, .cycles = 4, .exec = ldx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CLV", .addr_mode = IMPLIED, .cycles = 2, .exec = clv},
    {.mnemonic = "LDA", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = lda},
    {.mnemonic = "TSX", .addr_mode = IMPLIED, .cycles = 2, .exec = tsx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "LDY", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = ldy},
    {.mnemonic = "LDA", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = lda},
    {.mnemonic = "LDX", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = ldx},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CPY", .addr_mode = IMMEDIATE, .cycles = 2, .exec = cpy},
    {.mnemonic = "CMP", .addr_mode = INDIRECT_X, .cycles = 6, .exec = cmp},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CPY", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = cpy},
    {.mnemonic = "CMP", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = cmp},
    {.mnemonic = "DEC", .addr_mode = ZERO_PAGE, .cycles = 5, .exec = dec},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "INY", .addr_mode = IMPLIED, .cycles = 2, .exec = iny},
    {.mnemonic = "CMP", .addr_mode = IMMEDIATE, .cycles = 2, .exec = cmp},
    {.mnemonic = "DEX", .addr_mode = IMPLIED, .cycles = 2, .exec = dex},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CPY", .addr_mode = ABSOLUTE, .cycles = 4, .exec = cpy},
    {.mnemonic = "CMP", .addr_mode = ABSOLUTE, .cycles = 4, .exec = cmp},
    {.mnemonic = "DEC", .addr_mode = ABSOLUTE, .cycles = 6, .exec = dec},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BNE", .addr_mode = IMPLIED, .cycles = 2, .exec = bne},
    {.mnemonic = "CMP", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = cmp},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CMP", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = cmp},
    {.mnemonic = "DEC", .addr_mode = ZERO_PAGE_X, .cycles = 6, .exec = dec},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CLD", .addr_mode = IMPLIED, .cycles = 2, .exec = cld},
    {.mnemonic = "CMP", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = cmp},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CMP", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = cmp},
    {.mnemonic = "DEC", .addr_mode = ABSOLUTE_X, .cycles = 7, .exec = dec},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CPX", .addr_mode = IMMEDIATE, .cycles = 2, .exec = cpx},
    {.mnemonic = "SBC", .addr_mode = INDIRECT_X, .cycles = 6, .exec = sbc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CPX", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = cpx},
    {.mnemonic = "SBC", .addr_mode = ZERO_PAGE, .cycles = 3, .exec = sbc},
    {.mnemonic = "INC", .addr_mode = ZERO_PAGE, .cycles = 5, .exec = inc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "INX", .addr_mode = IMPLIED, .cycles = 2, .exec = inx},
    {.mnemonic = "SBC", .addr_mode = IMMEDIATE, .cycles = 2, .exec = sbc},
    {.mnemonic = "NOP", .addr_mode = IMPLIED, .cycles = 2, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "CPX", .addr_mode = ABSOLUTE, .cycles = 4, .exec = cpx},
    {.mnemonic = "SBC", .addr_mode = ABSOLUTE, .cycles = 4, .exec = sbc},
    {.mnemonic = "INC", .addr_mode = ABSOLUTE, .cycles = 6, .exec = inc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "BEQ", .addr_mode = IMPLIED, .cycles = 2, .exec = beq},
    {.mnemonic = "SBC", .addr_mode = INDIRECT_Y, .cycles = 5, .exec = sbc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "SBC", .addr_mode = ZERO_PAGE_X, .cycles = 4, .exec = sbc},
    {.mnemonic = "INC", .addr_mode = ZERO_PAGE_X, .cycles = 6, .exec = inc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "SED", .addr_mode = IMPLIED, .cycles = 2, .exec = sed},
    {.mnemonic = "SBC", .addr_mode = ABSOLUTE_Y, .cycles = 4, .exec = sbc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
    {.mnemonic = "SBC", .addr_mode = ABSOLUTE_X, .cycles = 4, .exec = sbc},
    {.mnemonic = "INC", .addr_mode = ABSOLUTE_X, .cycles = 7, .exec = inc},
    {.mnemonic = "???", .addr_mode = IMPLIED, .cycles = 0, .exec = nop},
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
    print_inst(cpu, &operand, &inst);

    inst.exec(cpu, &operand);
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

static void print_inst(const Cpu* cpu, const operand_t* operand, const Instruction* inst) {
    printf("$%x:\t%s  ", cpu->r_pc, inst->mnemonic);

    switch (operand->addr_mode) {
        case IMPLIED:
            break;
        case ACCUMULATOR:
            printf("A\n");
            break;
        case IMMEDIATE:
            printf("#%d\n", operand->val);
            break;
        case ZERO_PAGE:
            printf("$%x\n", operand->addr);
            break;
        case ZERO_PAGE_X:
            printf("$%x, X\n", operand->addr);
            break;
        case ZERO_PAGE_Y:
            printf("$%x, Y\n", operand->addr);
            break;
        case RELATIVE:
            printf("*%d\n", ((i8) operand->val));
            break;
        case ABSOLUTE:
            printf("$%x\n", operand->addr);
            break;
        case ABSOLUTE_X:
            printf("$%x, X\n", operand->addr);
            break;
        case ABSOLUTE_Y:
            printf("$%x, Y\n", operand->addr);
            break;
        case INDIRECT:
            printf("($%x)\n", operand->addr);
            break;
        case INDIRECT_X:
            printf("($%x, X)\n", operand->addr);
            break;
        case INDIRECT_Y:
            printf("($%x, Y)\n", operand->addr);
            break;
    }
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

static void bit(Cpu* cpu, operand_t* operand) {
    const u8 result = cpu->r_a & operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, OVERFLOW_FLAG, operand->val & 0x40);
    set_cpu_flag(cpu, NEGATIVE_FLAG, operand->val & 0x80);
}

static void bmi(Cpu* cpu, operand_t* operand) {
    if (get_cpu_flag(cpu, NEGATIVE_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static void bne(Cpu* cpu, operand_t* operand) {
    if (!get_cpu_flag(cpu, ZERO_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static void bpl(Cpu* cpu, operand_t* operand) {
    if (!get_cpu_flag(cpu, NEGATIVE_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

//TODO: Should we move the pc before/after incrementing?
static void brk(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    u8 msb_pc, lsb_pc;
    write_little_endian_u16(cpu->r_pc, &lsb_pc, &msb_pc);

    push_stack(cpu, lsb_pc);
    push_stack(cpu, msb_pc);
    push_stack(cpu, cpu->r_sr);

    set_cpu_flag(cpu, BREAK_COMMAND, true);
    cpu->r_pc = irq_interrupt_vector(cpu->mem);
}

static void bvc(Cpu* cpu, operand_t* operand) {
    if (!get_cpu_flag(cpu, OVERFLOW_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static void bvs(Cpu* cpu, operand_t* operand) {
    if (get_cpu_flag(cpu, OVERFLOW_FLAG)) {
        const i8 relative_val = read_u8(cpu, cpu->r_pc + 1);
        const u8 msb = (cpu->r_pc >> 8) & 0x00FF;
        cpu->r_pc += relative_val;
        operand->extra_cycles = msb != ((cpu->r_pc >> 8) & 0xFF) ? 2 : 1;
    }
}

static inline void clc(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, CARRY_FLAG, false);
}

static inline void cld(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, DECIMAL_FLAG, false);
}

static inline void cli(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, INTERRUPT_DISABLED_FLAG, false);
}

static inline void clv(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, OVERFLOW_FLAG, false);
}

static void cmp(Cpu* cpu, operand_t* operand) {
    const u16 result = (u16) cpu->r_a - (u16) operand->val;

    set_cpu_flag(cpu, CARRY_FLAG, cpu->r_a >= operand->val);
    set_cpu_flag(cpu, ZERO_FLAG, (result & 0x00FF) == 0x0000);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x0080);
}

static void cpx(Cpu* cpu, operand_t* operand) {
    const u16 result = (u16) cpu->r_x - (u16) operand->val;

    set_cpu_flag(cpu, CARRY_FLAG, cpu->r_x >= operand->val);
    set_cpu_flag(cpu, ZERO_FLAG, (result & 0x00FF) == 0x0000);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x0080);
}

static void cpy(Cpu* cpu, operand_t* operand) {
    const u16 result = (u16) cpu->r_y - (u16) operand->val;

    set_cpu_flag(cpu, CARRY_FLAG, cpu->r_y >= operand->val);
    set_cpu_flag(cpu, ZERO_FLAG, (result & 0x00FF) == 0x0000);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x0080);
}

static void dec(Cpu* cpu, operand_t* operand) {
    const u16 result = ((u16) operand->val) - 1;
    write_u8(cpu, operand->addr, result & 0x00FF);

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);
}

static void dex(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    const u16 result = ((u16) cpu->r_x) - 1;
    cpu->r_x = result & 0x00FF;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);
}

static void dey(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    const u16 result = ((u16) cpu->r_y) - 1;
    cpu->r_y = result & 0x00FF;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);
}

static void eor(Cpu* cpu, operand_t* operand) {
    const u8 result = cpu->r_a ^ operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);

    cpu->r_a = result;
}

static void inc(Cpu* cpu, operand_t* operand) {
    const u16 result = ((u16) operand->val) + 1;
    write_u8(cpu, operand->addr, result & 0x00FF);

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);
}

static void inx(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    const u16 result = ((u16) cpu->r_x) + 1;
    cpu->r_x = result & 0x00FF;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);
}

static void iny(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    const u16 result = ((u16) cpu->r_y) + 1;
    cpu->r_y = result & 0x00FF;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);
}

//FIXME: Watch out for the jump addresses
static void jmp(Cpu* cpu, operand_t* operand) {
    cpu->r_pc = operand->addr;
}

static void jsr(Cpu* cpu, operand_t* operand) {
    u8 msb_pc, lsb_pc;
    write_little_endian_u16(cpu->r_pc, &lsb_pc, &msb_pc);

    push_stack(cpu, lsb_pc);
    push_stack(cpu, msb_pc);

    cpu->r_pc = operand->addr;
}

static void lda(Cpu* cpu, operand_t* operand) {
    cpu->r_a = operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_a == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_a & 0x80);
}

static void ldx(Cpu* cpu, operand_t* operand) {
    cpu->r_x = operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_x == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_x & 0x80);
}

static void ldy(Cpu* cpu, operand_t* operand) {
    cpu->r_y = operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_y == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_y & 0x80);
}

static void lsr(Cpu* cpu, operand_t* operand) {
    u8 old_val, new_val;

    if (operand->addr_mode == ACCUMULATOR) {
        old_val = cpu->r_a;
        cpu->r_a = new_val = cpu->r_a >> 1;
    } else {
        old_val = operand->val;
        new_val = operand->val >> 1;
        write_u8(cpu, operand->addr, new_val);
    }

    set_cpu_flag(cpu, CARRY_FLAG, old_val & 0x01);
    set_cpu_flag(cpu, ZERO_FLAG, new_val == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, new_val & 0x80);
}

static inline void nop(Cpu __attribute__((__unused__)) *cpu, operand_t __attribute__((__unused__)) *operand) {}

static void ora(Cpu* cpu, operand_t* operand) {
    const u8 result = cpu->r_a | operand->val;

    set_cpu_flag(cpu, ZERO_FLAG, result == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, result & 0x80);

    cpu->r_a = result;
}

static inline void pha(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    push_stack(cpu, cpu->r_a);
}

static inline void php(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    push_stack(cpu, cpu->r_sr);
}

static void pla(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    pop_stack(cpu, &(cpu->r_a));

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_a == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_a & 0x80);
}

static inline void plp(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    pop_stack(cpu, &(cpu->r_sr));
}

static void rol(Cpu* cpu, operand_t* operand) {
    u8 old_val, new_val;

    if (operand->addr_mode == ACCUMULATOR) {
        old_val = cpu->r_a;
        cpu->r_a = new_val = (cpu->r_a << 1) | get_cpu_flag(cpu, CARRY_FLAG);
    } else {
        old_val = operand->val;
        new_val = operand->val << 1 | get_cpu_flag(cpu, CARRY_FLAG);
        write_u8(cpu, operand->addr, new_val);
    }

    set_cpu_flag(cpu, CARRY_FLAG, old_val >> 7);
    set_cpu_flag(cpu, ZERO_FLAG, new_val == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, new_val & 0x80);
}

static void ror(Cpu* cpu, operand_t* operand) {
    u8 old_val, new_val;

    if (operand->addr_mode == ACCUMULATOR) {
        old_val = cpu->r_a;
        cpu->r_a = new_val = cpu->r_a >> 1 | (get_cpu_flag(cpu, CARRY_FLAG) << 7);
    } else {
        old_val = operand->val;
        new_val = operand->val >> 1 | (get_cpu_flag(cpu, CARRY_FLAG) << 7);
        write_u8(cpu, operand->addr, new_val);
    }

    set_cpu_flag(cpu, CARRY_FLAG, old_val & 0x01);
    set_cpu_flag(cpu, ZERO_FLAG, new_val == 0x00);
    set_cpu_flag(cpu, NEGATIVE_FLAG, new_val & 0x80);
}

static void rti(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    u8 msb_pc = 0, lsb_pc = 0;

    pop_stack(cpu, &msb_pc);
    pop_stack(cpu, &lsb_pc);

    cpu->r_pc = read_little_endian_u16(lsb_pc, msb_pc);
}

static void rts(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    u8 msb_pc = 0, lsb_pc = 0;

    pop_stack(cpu, &msb_pc);
    pop_stack(cpu, &lsb_pc);

    cpu->r_pc = read_little_endian_u16(lsb_pc, msb_pc);
}

static void sbc(Cpu* cpu, operand_t* operand) {
    const u16 subtract = ((u16) cpu->r_a) - operand->val - ((u16) !get_cpu_flag(cpu, CARRY_FLAG)); 

    //FIXME: Not sure the carry flag will be set correctly
    set_cpu_flag(cpu, CARRY_FLAG, subtract & 0xFF00);
    set_cpu_flag(cpu, ZERO_FLAG, subtract == 0x0000);
    set_cpu_flag(cpu, NEGATIVE_FLAG, subtract & 0x0080);
    set_cpu_flag(cpu, OVERFLOW_FLAG, (~((uint16_t) cpu->r_a ^ (uint16_t) operand->val) & ((uint16_t) cpu->r_a ^ (uint16_t) subtract)) & 0x0080);
    
    cpu->r_a = subtract & 0x00FF;
}

static inline void sec(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, CARRY_FLAG, true);
}

static inline void sed(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, DECIMAL_FLAG, true);
}

static inline void sei(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    set_cpu_flag(cpu, INTERRUPT_DISABLED_FLAG, true);
}

static inline void sta(Cpu* cpu, operand_t* operand) {
    write_u8(cpu, operand->addr, cpu->r_a);
}

static inline void stx(Cpu* cpu, operand_t* operand) {
    write_u8(cpu, operand->addr, cpu->r_x);
}

static inline void sty(Cpu* cpu, operand_t* operand) {
    write_u8(cpu, operand->addr, cpu->r_y);
}

static void set_cpu_flag(Cpu* cpu, StatusFlag flag, bool set) {
    if (set) {
        cpu->r_sr |= flag;
    }
    else {
        cpu->r_sr &= ~flag;
    }
}

static void tax(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    cpu->r_x = cpu->r_a;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_x == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_x & 0x80);
}

static void tay(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    cpu->r_y = cpu->r_a;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_y == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_y & 0x80);
}

static void tsx(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    cpu->r_x = cpu->r_sp;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_x == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_x & 0x80);
}

static void txa(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    cpu->r_a = cpu->r_x;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_a == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_a & 0x80);
}

static inline void txs(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    cpu->r_sp = cpu->r_x;
}

static void tya(Cpu* cpu, operand_t __attribute__((__unused__)) *operand) {
    cpu->r_a = cpu->r_y;

    set_cpu_flag(cpu, ZERO_FLAG, cpu->r_a == 0);
    set_cpu_flag(cpu, NEGATIVE_FLAG, cpu->r_a & 0x80);
}

static inline bool get_cpu_flag(Cpu* cpu, StatusFlag flag) {
    return (cpu->r_sr & flag) > 0;
}

//TODO: Check push pull statuses
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
