#include "je.h"
#include "cpu/reg.h"
#include "memory/memory.h"

make_helper(je) {
    uint32_t return_addr = swaddr_read(cpu.esp, 4);
    cpu.esp+=4;

    cpu.esp=return_addr;
    return 0;
}