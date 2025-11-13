#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"
#include "cpu/reg.h"

// mov r32, cr0 (0x0F 0x20)
make_helper(mov_r_cr0) {
    decode_rm_l(eip);
    op_dest->val = cpu.CR0.val;
    print_asm("mov %s, cr0", op_dest->str);
    return 1;
}

// mov cr0, r32 (0x0F 0x22)  
make_helper(mov_cr0_r) {
    decode_rm_l(eip);
    
    // 保存旧值用于调试
    uint32_t old_cr0 = cpu.CR0.val;
    cpu.CR0.val = op_src->val;
    
    // 保护模式切换调试信息
    if ((old_cr0 & 1) != (cpu.CR0.val & 1)) {
        printf("Protection mode %s\n", (cpu.CR0.val & 1) ? "enabled" : "disabled");
    }
    
    print_asm("mov cr0, %s", op_src->str);
    return 1;
}