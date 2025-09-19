#include "cpu/exec/helper.h"
#include "cpu/reg.h"
#include "push.h"
#include "memory/memory.h" 

make_helper(push_r_v) {
    int reg_index=ops_decoded.opcode & 0x7; //获取寄存器编号
    uint32_t val=reg_l(reg_index); 

    cpu.esp -= 4;        
    *(uint32_t *)(hw_mem + cpu.esp) = val;

    print_asm("pushl %s", regsl[reg_index]);
    return 1;
}

make_helper(pop_r_v) {
    int reg_index = ops_decoded.opcode & 0x7;
    uint32_t val = *((uint32_t *)hw_mem + cpu.esp);
    cpu.esp += 4;
    reg_l(reg_index) = val;

    print_asm("pop %s", regsl[reg_index]);
    return 1;
}
