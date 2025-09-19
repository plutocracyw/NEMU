#include "cmp.h"
#include "cpu/exec/helper.h"
#include "cpu/eflags.h"
#include "cpu/decode/modrm.h"

make_helper(cmp_b) {
    uint8_t src=instr_fetch(eip+1,1);
    uint8_t dest=cpu.eax & 0xff;
    uint16_t result=(uint16_t)dest-(uint16_t)src;

    cpu.eflags.CF=(result>>8) & 1;
    cpu.eflags.ZF=((uint8_t)result==0);
    cpu.eflags.SF=((uint8_t)result>>7) & 1;
      cpu.eflags.OF = ((dest ^ src) & (dest ^ (uint8_t)result) & 0x80) ? 1 : 0;

    return 2;
}

make_helper(cmp_v) {
    uint32_t src=instr_fetch(eip+1,4);
    uint32_t dest=cpu.eax;
    uint64_t result=(uint64_t)dest-(uint64_t)src;

    cpu.eflags.CF=(result>>32)&1;
    cpu.eflags.ZF=((uint32_t)result==0);
    cpu.eflags.SF=((uint32_t)result>>31)&1;
    cpu.eflags.OF=(((dest^src)&(dest^(uint32_t)result))>>31)&1;

    return 5;
}