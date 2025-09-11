#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	char expr[256];      // 用于存储表达式字符串
	uint32_t value;      // 用于存储表达式的旧值

} WP;

WP* new_wp();
void free_wp(WP *wp);
bool delete_wp(int N);
void display_watchpoints();


#endif
