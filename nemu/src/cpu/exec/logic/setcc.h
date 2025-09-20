#ifndef __SETCC_H__
#define __SETCC_H__

#include "cpu/reg.h"
#include <stdint.h>

/* 条件码检查函数声明 */
int check_cc_e(void);
int check_cc_ne(void);
int check_cc_g(void);
int check_cc_ge(void);
int check_cc_l(void);
int check_cc_le(void);
int check_cc_s(void);
int check_cc_ns(void);
int check_cc_a(void);
int check_cc_be(void);
int check_cc_b(void);

/* setcc 指令处理函数声明 */
void setcc(int cc, uint32_t *dest);

#endif /* __SETCC_H__ */
