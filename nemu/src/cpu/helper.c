#include "common.h"
#include "cpu/reg.h"
#include "cpu/cpu.h"
#include "cpu/intr.h"

// ---------------------------
// 加载 GDTR
// ---------------------------
void lgdt_v(uint32_t addr, uint32_t len) {
    cpu.GDTR.limit = swaddr_read(addr, 2);
    cpu.GDTR.base  = swaddr_read(addr + 2, 4);
}

// ---------------------------
// 加载 IDTR
// ---------------------------
void lidt_v(uint32_t addr, uint32_t len) {
    cpu.IDTR.limit = swaddr_read(addr, 2);
    cpu.IDTR.base  = swaddr_read(addr + 2, 4);
}

// ---------------------------
// 触发中断
// ---------------------------
void raise_intr_v(uint8_t NO, uint32_t ret_addr) {
    raise_intr(NO, ret_addr);
}

// ---------------------------
// 从中断返回
// ---------------------------
void iret_v() {
    iret();
}
