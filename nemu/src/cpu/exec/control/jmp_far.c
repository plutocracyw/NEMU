#include "cpu/exec/helper.h"
#include "cpu/reg.h"

make_helper(jmp_far) {
    // 读取 4 字节偏移和 2 字节段选择子
    uint32_t offset = instr_fetch(eip, 4);
    uint16_t selector = instr_fetch(eip + 4, 2);
    
    // 设置 CS 和 EIP
    cpu.eip = offset;
    cpu.CS.selector = selector;
    
    // 更新 CS 隐藏部分
    if (cpu.CR0.PE) {
        // 保护模式
        cpu.CS.base = 0;
        cpu.CS.limit = 0xFFFFFFFF;
        cpu.CS.present = 1;
    } else {
        // 实模式
        cpu.CS.base = selector << 4;
        cpu.CS.limit = 0xFFFF;
        cpu.CS.present = 1;
    }
    
    print_asm("jmp $0x%x, $0x%x", selector, offset);
    return 6;
}