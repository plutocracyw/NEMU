#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */

char* rl_gets() 
{
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

//计算
static int cmd_expr(char *args) {
    bool success = true;
    uint32_t result = expr(args, &success);  // 调用expr()
    if (success) 
	{
        printf("%u\n", result);  
    } 
	else 
	{
        printf("Invalid expression: %s\n", args);
    }
    return 0;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

//单步执行命令函数
static int cmd_si(char *args){
	int n=1;
	if (args != NULL) {
        sscanf(args, "%d", &n);
    }
	cpu_exec(1);
	return 0;
}


//打印寄存器命令函数
static int cmd_info(char *args){
	if (args == NULL) {
        printf("Usage: info r | info w\n");
        return 0;
    }

	else if(strcmp(args,"r")==0){
		printf("eax\t0x%08x\n", cpu.eax);
		printf("ecx\t0x%08x\n", cpu.ecx);
		printf("edx\t0x%08x\n", cpu.edx);
		printf("ebx\t0x%08x\n", cpu.ebx);
		printf("esp\t0x%08x\n", cpu.esp);
		printf("ebp\t0x%08x\n", cpu.ebp);
		printf("esi\t0x%08x\n", cpu.esi);
		printf("edi\t0x%08x\n", cpu.edi);
		printf("eip\t0x%08x\n", cpu.eip);
		printf("eflags\t0x%08x\n", cpu.eflags.val);
	}
	else if(strcmp(args,"w")==0){
		printf("watchpoints not implemented yet\n");
	}
	else if (strcmp(args, "w") == 0) {
        display_watchpoints(); 
    }
	else {
        printf("Unknown info subcommand '%s'\n", args);
    }
    return 0;
	
}


//扫描内存函数
static int cmd_x(char *args){
	char*arg=strtok(args," ");
	if(arg==NULL)
	{
		printf("Usage: x [length] [address]\n");
		return 0;
	}

	int len=atoi(arg);
	arg=strtok(NULL," ");
	if(arg==NULL)
	{
		 printf("Usage: x [length] [address]\n");
		 return 0;
	}

	uint32_t addr;
    sscanf(arg, "%x", &addr);

	for(int i=0;i<len;i+=4)
	{
		uint32_t data=hwaddr_read(addr+i,4);
		printf("0x%08x: 0x%08x\n", addr + i, data);
	}
	return 0;
}

// 在 ui.c 中

static int cmd_w(char *args) {
    if (args == NULL) {
        printf("Usage: w EXPRESSION\n");
        return 0;
    }

    bool success;
    WP* wp = new_wp();
    if (wp == NULL) {
        printf("Failed to create new watchpoint. The pool might be full.\n");
        return 0;
    }

    if (strlen(args) >= sizeof(wp->expr)) {
        printf("Error: Expression is too long (max %zu characters).\n", sizeof(wp->expr) - 1);
        delete_wp(wp->NO);
        return 0;
    }
    strcpy(wp->expr, args); 


    wp->value = expr(args, &success);
    if (!success) {
        printf("Invalid expression. Deleting the watchpoint.\n");
        delete_wp(wp->NO);
        return 0;
    }

    printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
    return 0;
}

static int cmd_d(char *args) {
	if (args == NULL) {
		printf("Usage: d N(where N is the watchpoint number)\n");
		return 0;
	}

	int n;
	sscanf(args, "%d", &n);
	if (delete_wp(n)) {
		printf("Delete watchpoint %d.\n", n);
	}
	else {
		printf("Watchpoint %d not found\n", n);
	}
	return 0;
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} 

//命名表
cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Step one instruction", cmd_si},
	{ "info", "Prinf register values", cmd_info},
	{ "x", "Scan memory", cmd_x},
	{ "expr", "Evaluate the expression", cmd_expr },
	{ "w", "Set a watchpoint. Usage: w EXPR", cmd_w },
    { "d", "Delete a watchpoint. Usage: d N", cmd_d },


	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) {
			 printf("Unknown command '%s'\n", cmd);
		}

WP* new_wp();
bool delete_wp(int N);
void display_watchpoints();

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif
	}
}
		

