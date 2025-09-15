#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"
#include <stdint.h>

typedef struct watchpoint {
    int NO;
    struct watchpoint *next;
    char expr[256];    // 表达式
    uint32_t value;    // 旧值
} WP;

WP* new_wp(void);
void free_wp(WP *wp);
bool delete_wp(int N);
void display_watchpoints(void);
bool check_watchpoints(void);
void init_wp_pool(void);

#endif
