#include "cpu/reg.h"
#include "cpu/exec/helper.h"
#include "memory/memory.h"
#include "cpu/decode/decode.h"
#include "common.h"

make_helper(call_si) {
    int len = decode_si_l(eip + 1); 
    swaddr_t ret_addr = cpu.eip + len + 1; 

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, ret_addr);

    cpu.eip += op_src->val;
    print_asm("call 0x%x", cpu.eip);

    return len+1; 
}

make_helper(call_rm) {
    int len = decode_rm_l(eip + 1);  
    swaddr_t ret_addr = cpu.eip + len + 1;

    cpu.esp -= 4;
    swaddr_write(cpu.esp , 4, ret_addr);

    cpu.eip = op_src->val;
    print_asm("call *%s", op_src->str);

    return len+1; 
}
