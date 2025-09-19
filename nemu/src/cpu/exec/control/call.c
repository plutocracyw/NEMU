#include "call.h"
#include "cpu/reg.h"
#include "memory/memory.h"

make_helper(call) {
    uint32_t dest = instr_fetch(eip + 1, 4); // 获取立即数地址
    uint32_t return_addr = eip + 5;          // call 长度为5字节
    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, return_addr);

    cpu.eip = dest;
    return 0;
}

make_helper(call_rm_v) {
    uint32_t dest = instr_fetch(eip + 1, 4);
    uint32_t return_addr = eip + 5;

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, return_addr);

    cpu.eip = dest;
    return 0;
}
