#include "cpu/exec/helper.h"
#include "cpu/reg.h"
#include "memory/memory.h"
#include "cpu/decode/modrm.h"

static inline uint32_t do_pop_32(void) {
  uint32_t val = swaddr_read(cpu.esp, 4);
  cpu.esp += 4;
  return val;
}
make_helper(pop_r_v) {
  uint8_t opcode = instr_fetch(eip, 1);
  int rd = opcode & 0x7;

  uint32_t v = do_pop_32();
  reg_l(rd) = v;

  print_asm("pop %%%s", regsl[rd]);
  return 1; // 只有 1 字节 opcode
}

make_helper(pop_rm_v) {
  // 读取 ModR/M
  uint8_t modrm = instr_fetch(eip + 1, 1);
  ModR_M m; m.val = modrm;
  int len = 1; 

  if (m.mod == 3) {
    // 目标是寄存器
    int r = m.R_M;
    uint32_t v = do_pop_32();
    reg_l(r) = v;

    print_asm("pop %%%s", regsl[r]);
    return 1 + len; 
  } else {
    // 目标是内存
    swaddr_t addr = 0;

    if (m.mod == 0 && m.R_M == 5) {
      addr = instr_fetch(eip + 2, 4); // 读取 disp32
      len += 4;
    } else {
      panic("pop r/m: addressing mode not implemented (mod=%u r/m=%u)", m.mod, m.R_M);
    }

    uint32_t v = do_pop_32();
    swaddr_write(addr, 4, v);

    print_asm("pop 0x%x", addr);
    return 1 + len; 
  }
}
