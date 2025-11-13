#ifndef __INTR_H__
#define __INTR_H__

#include "common.h"
#include "cpu/reg.h"
#include "memory/memory.h"  // swaddr_read/swaddr_write

// ---------------------------
// raise_intr: 触发中断
// ---------------------------
static inline void raise_intr(uint8_t NO, swaddr_t ret_addr) {
    // 中断门地址 = IDT 基址 + 中断号 * 8
    uint32_t gate_addr = cpu.IDTR.base + NO * 8;

    // 读取中断门描述符低 / 高 4 字节
    uint32_t low  = swaddr_read(gate_addr, 4);
    uint32_t high = swaddr_read(gate_addr + 4, 4);

    // 拼出偏移地址
    uint32_t offset = (low & 0xFFFF) | (high & 0xFFFF0000);
    uint16_t selector = (low >> 16) & 0xFFFF;

    // 压栈保存 eflags, CS, EIP
    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, cpu.eflags.val);

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, cpu.CS.selector);

    cpu.esp -= 4;
    swaddr_write(cpu.esp, 4, ret_addr);

    // 跳转到中断处理例程
    cpu.CS.selector = selector;
    cpu.eip = offset;
}

// ---------------------------
// iret: 从中断返回
// ---------------------------
static inline void iret(void) {
    cpu.eip = swaddr_read(cpu.esp, 4);
    cpu.esp += 4;

    cpu.CS.selector = swaddr_read(cpu.esp, 4);
    cpu.esp += 4;

    cpu.eflags.val = swaddr_read(cpu.esp, 4);
    cpu.esp += 4;
}

#endif
