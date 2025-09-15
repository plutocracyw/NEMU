#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "cpu/reg.h"
#include <stdio.h>
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head = NULL;
static WP *free_ = NULL;

/* 初始化 watchpoint 池 */
void init_wp_pool(void) {
    int i;
    for (i = 0; i < NR_WP - 1; i++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
    }
    wp_pool[NR_WP - 1].NO = NR_WP - 1;
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
}

/* 创建新的 watchpoint */
WP* new_wp(void) {
    WP *wp;

    if (free_ == NULL) {
        printf("Watchpoint pool is full. Cannot create a new watchpoint.\n");
        return NULL;
    }

    wp = free_;
    free_ = free_->next;

    wp->next = head;
    head = wp;

    wp->expr[0] = '\0';
    wp->value = 0;

    return wp;
}

/* 释放 watchpoint */
void free_wp(WP *wp) {
    WP *current = head;
    WP *prev = NULL;

    if (wp == NULL) {
        printf("Attempting to free a NULL watchpoint.\n");
        return;
    }

    while (current != NULL && current != wp) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Attempting to free a watchpoint not in use.\n");
        return;
    }

    if (prev == NULL) {
        head = wp->next;
    } else {
        prev->next = wp->next;
    }

    wp->next = free_;
    free_ = wp;
}

/* 删除指定编号的 watchpoint */
bool delete_wp(int N) {
    WP *wp = head;
    while (wp != NULL) {
        if (wp->NO == N) {
            free_wp(wp);
            return 1; /* true */
        }
        wp = wp->next;
    }
    return 0; /* false */
}

/* 显示所有 watchpoint */
void display_watchpoints(void) {
    WP *wp = head;
    if (wp == NULL) {
        printf("No watchpoints set.\n");
        return;
    }

    printf("Num\tWhat\t\tOld Value\n");
    while (wp != NULL) {
        printf("%d\t%s\t\t%u\n", wp->NO, wp->expr, wp->value);
        wp = wp->next;
    }
}

/* 检查每条指令后 watchpoint 是否命中 */
bool check_watchpoints(void) {
    WP *wp = head;
    bool hit = 0;

    while (wp != NULL) {
        bool success;
        uint32_t new_val = expr(wp->expr, &success);

        if (!success) {
            printf("Watchpoint %d evaluation failed: %s\n", wp->NO, wp->expr);
            wp = wp->next;
            continue;
        }

        if (new_val != wp->value) {
            printf("Watchpoint %d triggered!\n", wp->NO);
            printf("Old value: 0x%08x, New value: 0x%08x\n", wp->value, new_val);
            wp->value = new_val;
            hit = 1;
        }

        wp = wp->next;
    }

    return hit;
}
