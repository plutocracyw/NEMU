#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

#define EXPR_MAX_LEN 128


typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[128];
  uint32_t value;
} WP;

void init_wp_pool(void);
WP* new_wp(const char *expr);
void free_wp(WP *wp);
bool delete_wp(int NO);
void display_watchpoints(void);
bool check_watchpoints(void);

#endif