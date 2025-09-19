#include "ret.h"
#include "cpu/reg.h"
#include "memory/memory.h"

make_helper(ret) {
    uint32_t addr = swaddr_read(cpu.esp, 4);  // 从栈顶取返回地址
    cpu.esp += 4; 
    cpu.eip = addr; 
    return 1; 
}
