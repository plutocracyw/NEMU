#ifndef __CPU_H__
#define __CPU_H__

#include "common.h"
#include "cpu/reg.h"

// 初始化 CPU 状态
static inline void init_cpu() {
    cpu.eip = 0;
    cpu.eflags.val = 0x2; // 初始 EFLAGS
    cpu.CR0.val = 0;      // 实模式
    cpu.GDTR.base = 0;
    cpu.GDTR.limit = 0;
    cpu.CS.selector = 0;
    cpu.DS.selector = 0;
    cpu.ES.selector = 0;
    cpu.SS.selector = 0;
}

#endif
