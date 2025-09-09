#include "nemu.h"

#include "cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include "cpu-exec.h"

int cmd_si(char * num) {
    int n = 1;
    if (num != NULL) {
        sscanf(num, "%d", &n);
    }

    cpu_exec(n); 
    return 0;
}