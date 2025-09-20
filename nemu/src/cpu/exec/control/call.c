#include "cpu/reg.h"
#include "cpu/exec/helper.h"
#include "memory/memory.h"
#include "cpu/decode/decode.h"
#include "common.h"

make_helper(call_si) {
    int len = decode_si_l(eip + 1);  // 解码立即数操作数
    swaddr_t ret_addr = cpu.eip + len + 1; 
    swaddr_write(cpu.esp - 4, 4, ret_addr);
    cpu.esp -= 4;
    cpu.eip += op_src->val;

    // 打印指令信息
    print_asm("call 0x%x", cpu.eip);

    return len + 1; 
}

make_helper(call_rm) {
    int len = decode_rm_l(eip + 1);  
    swaddr_t ret_addr = cpu.eip + len + 1;
    swaddr_write(cpu.esp - 4, 4, ret_addr);
    cpu.esp -= 4; 
    cpu.eip = op_src->val;

    print_asm("call *%s", op_src->str);

    return len + 1; 
}
