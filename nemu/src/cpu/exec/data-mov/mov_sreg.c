#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"
#include "cpu/reg.h"

make_helper(mov_sreg_rm16) {
    ModR_M m;
    m.val = instr_fetch(eip, 1);
    
    int len = 1;
    uint16_t selector;
    
    if (m.mod == 3) {
        // 寄存器源操作数
        selector = reg_w(m.R_M);
        len += 1;
    } else {
        // 内存源操作数
        Operand rm;
        len += read_ModR_M(eip + 1, &rm, NULL);
        selector = swaddr_read(rm.addr, 2);
    }
    
    // 根据 ModR/M 的 reg 字段设置对应的段寄存器
    SegReg *target_sreg;
    const char *sreg_name;
    switch (m.reg) {
        case 0: target_sreg = &cpu.ES; sreg_name = "es"; break;
        case 1: target_sreg = &cpu.CS; sreg_name = "cs"; break;
        case 2: target_sreg = &cpu.SS; sreg_name = "ss"; break;
        case 3: target_sreg = &cpu.DS; sreg_name = "ds"; break;
        case 4: target_sreg = &cpu.FS; sreg_name = "fs"; break;
        case 5: target_sreg = &cpu.GS; sreg_name = "gs"; break;
        default: assert(0);
    }
    
    // 设置段选择子
    target_sreg->selector = selector;
    
    // 更新段寄存器隐藏部分
    if (cpu.CR0.PE) {
        // 保护模式：简化实现，假设平坦模型
        target_sreg->base = 0;
        target_sreg->limit = 0xFFFFFFFF;
        target_sreg->present = 1;
    } else {
        // 实模式
        target_sreg->base = selector << 4;
        target_sreg->limit = 0xFFFF;
        target_sreg->present = 1;
    }
    
    // 修复：移除对 ModR_M_asm 的引用
    const char *reg_names[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    if (m.mod == 3) {
        print_asm("mov %%%s, %%%s", sreg_name, reg_names[m.R_M]);
    } else {
        // 对于内存操作数，简化输出
        print_asm("mov %%%s, [mem]", sreg_name);
    }
    
    return len;
}