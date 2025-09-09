#ifndef CMD_H
#define CMD_H

#include <stdbool.h>

int cmd_si(char *args);
int cmd_info(char *args);
int cmd_x(char *args);
void ui_mainloop();

#endif
