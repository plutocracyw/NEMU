#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {

    Assert(free_ != NULL, "Watchpoint pool is full. Cannot create a new watchpoint.");

    WP *new_node = free_;
    free_ = free_->next;

    new_node->next = head;
    head = new_node;

    return new_node;
}

void free_wp(WP *wp) {
    if (wp == NULL) {
        printf("Error: Attempting to free a NULL watchpoint.\n");
        return;
    }

    WP *current = head;
    WP *prev = NULL;

    while (current != NULL && current != wp) {
        prev = current;
        current = current->next;
    }

    Assert(current != NULL, "Attempting to free a watchpoint that is not in use.");

    if (prev == NULL) {
        head = wp->next;
    } else {
        prev->next = wp->next;
    }

    wp->next = free_;
    free_ = wp;
}

bool delete_wp(int N) {
    WP *wp = head;
    while (wp != NULL) {
        if (wp->NO == N) {
            free_wp(wp);
            return true;
        }
        wp = wp->next;
    }
    return false;
}


void display_watchpoints() {
    if (head == NULL) {
        printf("No watchpoints set.\n");
        return;
    }

    printf("Num\tWhat\t\tOld Value\n");
    WP *wp = head;
    while (wp != NULL) {
        printf("%d\t%s\t\t%u\n", wp->NO, wp->expr, wp->value);
        wp = wp->next;
    }
}