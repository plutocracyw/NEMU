#include "cpu/exec/template-start.h"

#define instr adc

static void do_execute () {
	uint32_t old_CF = cpu.eflags.CF;

	DATA_TYPE result = op_dest->val + op_src->val + cpu.eflags.CF;
	OPERAND_W(op_dest, result);

	/* TODO: Update EFLAGS. */
	 update_eflags_pf_zf_sf((DATA_TYPE_S)result);

    // 更新 AF（辅助进位）
    cpu.eflags.AF = ((op_dest->val & 0xF) + (op_src->val & 0xF) + old_CF) > 0xF;
	cpu.eflags.CF = result < op_dest->val;
	cpu.eflags.OF = MSB(~(op_dest->val ^ op_src->val) & (op_dest->val ^ result));

	print_asm_template2();
}

make_instr_helper(r2rm)

#include "cpu/exec/template-end.h"
