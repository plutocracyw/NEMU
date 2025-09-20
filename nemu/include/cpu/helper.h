#ifndef __HELPER_H__
#define __HELPER_H__

#include "nemu.h"
#include "cpu/decode/operand.h"
#include "cpu/eflags.h" 

/* All function defined with 'make_helper' return the length of the operation. */
#define make_helper(name) int name(swaddr_t eip)

static inline uint32_t instr_fetch(swaddr_t addr, size_t len) {
	return swaddr_read(addr, len);
}

/* Instruction Decode and EXecute */
static inline int idex(swaddr_t eip, int (*decode)(swaddr_t), void (*execute) (void)) {
	/* eip is pointing to the opcode */
	int len = decode(eip + 1);
	execute();
	return len + 1;	// "1" for opcode
}

// 条件跳转 make_helper 声明
make_helper(jb_b);
make_helper(jbe_b);
make_helper(ja_b);
make_helper(je_b);
make_helper(jne_b);
make_helper(jl_b);
make_helper(jge_b);
make_helper(jle_b);
make_helper(jg_b);
make_helper(js_b);
make_helper(jns_b);

// 如果有 32 位版本
make_helper(jb_l);
make_helper(jbe_l);
make_helper(ja_l);
make_helper(je_l);
make_helper(jne_l);
make_helper(jl_l);
make_helper(jge_l);
make_helper(jle_l);
make_helper(jg_l);
make_helper(js_l);
make_helper(jns_l);

// setcc 系列函数
make_helper(setne);  // ZF == 0
make_helper(sete);   // ZF == 1
make_helper(setg);   // >，有符号
make_helper(setge);  // >=，有符号
make_helper(setl);   // <，有符号
make_helper(setle);  // <=，有符号
make_helper(sets);   // SF == 1
make_helper(setns);  // SF == 0
make_helper(seta);   // CF==0 && ZF==0
make_helper(setbe);  // CF==1 || ZF==1
make_helper(setb);   // CF == 1


/* shared by all helper function */
extern Operands ops_decoded;

#define op_src (&ops_decoded.src)
#define op_src2 (&ops_decoded.src2)
#define op_dest (&ops_decoded.dest)


#endif
