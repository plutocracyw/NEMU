#include "cpu/exec/helper.h"
#include "cpu/reg.h"

// 通用的跳转执行
static inline int do_jcc(int condition, uint32_t eip, int offset_size) {
    // 取偏移
    int32_t offset = 0;
    if (offset_size == 1) {
        offset = (int8_t)instr_fetch(eip + 1, 1);
    } else {
        offset = (int32_t)instr_fetch(eip + 1, 4);
    }

    // 如果条件满足，跳转到指定的地址
    if (condition) {
        cpu.eip += offset;
    }
    return 1 + offset_size;
}

// 条件跳转的宏定义（1字节偏移）
make_helper(jb_b)   { return do_jcc(cpu.eflags.CF == 1, eip, 1); }  // 检查 CF
make_helper(je_b)   { return do_jcc(cpu.eflags.ZF == 1, eip, 1); }  // 检查 ZF
make_helper(jne_b)  { return do_jcc(cpu.eflags.ZF == 0, eip, 1); }  // 检查 ZF
make_helper(jbe_b)  { return do_jcc(cpu.eflags.CF == 1 || cpu.eflags.ZF == 1, eip, 1); }  // 检查 CF 或 ZF
make_helper(ja_b)   { return do_jcc(cpu.eflags.CF == 0 && cpu.eflags.ZF == 0, eip, 1); }  // 检查 CF 和 ZF
make_helper(js_b)   { return do_jcc(cpu.eflags.SF == 1, eip, 1); }  // 检查 SF
make_helper(jns_b)  { return do_jcc(cpu.eflags.SF == 0, eip, 1); }  // 检查 SF
make_helper(jl_b)   { return do_jcc(cpu.eflags.SF != cpu.eflags.OF, eip, 1); }  // 检查 SF 与 OF
make_helper(jge_b)  { return do_jcc(cpu.eflags.SF == cpu.eflags.OF, eip, 1); }  // 检查 SF 和 OF
make_helper(jle_b)  { return do_jcc(cpu.eflags.ZF == 1 || cpu.eflags.SF != cpu.eflags.OF, eip, 1); }  // 检查 ZF 或 SF != OF
make_helper(jg_b)   { return do_jcc(cpu.eflags.ZF == 0 && cpu.eflags.SF == cpu.eflags.OF, eip, 1); }  // 检查 ZF 和 SF == OF

// 32 位偏移版（_l）
make_helper(je_l)   { return do_jcc(cpu.eflags.ZF == 1, eip, 4); }  // 检查 ZF
make_helper(jne_l)  { return do_jcc(cpu.eflags.ZF == 0, eip, 4); }  // 检查 ZF
make_helper(jbe_l)  { return do_jcc(cpu.eflags.CF == 1 || cpu.eflags.ZF == 1, eip, 4); }  // 检查 CF 或 ZF
make_helper(ja_l)   { return do_jcc(cpu.eflags.CF == 0 && cpu.eflags.ZF == 0, eip, 4); }  // 检查 CF 和 ZF
make_helper(jl_l)   { return do_jcc(cpu.eflags.SF != cpu.eflags.OF, eip, 4); }  // 检查 SF 与 OF
make_helper(jge_l)  { return do_jcc(cpu.eflags.SF == cpu.eflags.OF, eip, 4); }  // 检查 SF 和 OF
make_helper(jle_l)  { return do_jcc(cpu.eflags.ZF == 1 || cpu.eflags.SF != cpu.eflags.OF, eip, 4); }  // 检查 ZF 或 SF != OF
