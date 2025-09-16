#ifndef __MONITOR_H__
#define __MONITOR_H__
#include <stdint.h>

void cpu_exec(uint32_t n);

enum { STOP, RUNNING, END };
extern int nemu_state;

#endif
