#include "cpu/exec/helper.h"
#include "cpu/reg.h"
#include "memory/memory.h"
#include "cpu/decode/modrm.h"

/* setne r/m8 : ZF==0 时把目标字节置 1，否则置 0 */
make_helper(setne) {
    uint8_t modrm = instr_fetch(eip + 1, 1);
    ModR_M m; m.val = modrm;
    int len = 1;                  // 已读取 1 字节 ModR/M
    uint8_t val = (cpu.eflags.ZF == 0) ? 1 : 0;

    if (m.mod == 3) {
        // 目标是寄存器
        reg_b(m.R_M) = val;
        print_asm("setne %%%s", regsb[m.R_M]);
        return 2;                 // opcode + modrm
    } else {
        // 目标是内存 —— 这里只实现最简单的 [disp32] 形式
        swaddr_t addr = 0;
        if (m.mod == 0 && m.R_M == 5) {
            addr = instr_fetch(eip + 2, 4);
            len += 4;
        } else {
            panic("setne: addressing mode not implemented (mod=%u r/m=%u)",
                  m.mod, m.R_M);
        }
        swaddr_write(addr, 1, val);
        print_asm("setne 0x%x", addr);
        return 1 + len;
    }
}
