#ifndef __POP_TEMPLATE_H__
#define __POP_TEMPLATE_H__

#include "cpu/exec/helper.h"
#include "cpu/reg.h"
#include "memory/memory.h"
#include "cpu/decode/modrm.h"
#include "cpu/decode/decode.h"

// 根据 DATA_BYTE 弹出值到 dest
static inline uint32_t do_pop_v(void) {
    uint32_t val = swaddr_read(cpu.esp, DATA_BYTE);
    cpu.esp += DATA_BYTE;
    return val;
}
make_helper(concat(pop_r_, SUFFIX)) {
    // opcode 就在 eip 处
    uint8_t opcode = instr_fetch(eip, 1);
    int reg = opcode & 0x7;
#if DATA_BYTE == 1
    reg_b(reg) = (uint8_t)do_pop_v();
#elif DATA_BYTE == 2
    reg_w(reg) = (uint16_t)do_pop_v();
#elif DATA_BYTE == 4
    reg_l(reg) = do_pop_v();
#endif
    print_asm("pop %%%s", regsl[reg]);   
    return 1;
}

make_helper(concat(pop_rm_, SUFFIX)) {
    decode_rm_v(eip + 1);      
    uint32_t val = do_pop_v();
    OPERAND_W(op_dest, val);     
    print_asm_template1(pop);    
    return 1 + op_src->len;   
}
