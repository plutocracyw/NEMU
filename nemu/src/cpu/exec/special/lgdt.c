#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"
#include "cpu/reg.h"

make_helper(lgdt) {
    ModR_M m;
    m.val = instr_fetch(eip, 1);
    
    // 使用正确的 Operand 结构
    Operand rm;
    int len = read_ModR_M(eip, &rm, NULL);
    
    // GDTR 格式: 2字节界限 + 4字节基址
    // 使用正确的 swaddr_read 调用（2个参数）
    cpu.GDTR.limit = swaddr_read(rm.addr, 2);
    cpu.GDTR.base = swaddr_read(rm.addr + 2, 4);
    
    printf("LGDT: base=0x%08x, limit=0x%04x\n", cpu.GDTR.base, cpu.GDTR.limit);
    
    // 修复：移除对 ModR_M_asm 的引用
    print_asm("lgdt [mem]");
    return len;
}