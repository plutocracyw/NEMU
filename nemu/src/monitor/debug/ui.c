#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>




/* We use the `readline' library to provide more flexibility to read from stdin. */

//读取用户输入
char* rl_gets(){
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");  //显示提示符 `(nemu) ` 并等待用户输入。

	if (line_read && *line_read) {
		add_history(line_read);
	}  //如果用户输入了非空字符串，就将其加入到历史记录中。

	return line_read;
}

/*rgs:参数字符串
 `c` (continue) 命令,连续执行，直到遇到断点、监视点被触发或程序结束。
 `q` (quit) 命令,退出 NEMU。
 `si [N]` (step-instruction) 命令,单步执行 NEMU 指令,默认执行 1 条指令。
 `info r` 命令,打印寄存器状态。
 `info w` 命令,打印所有监视点的信息。
 `x` (examine memory) 命令，用于扫描和显示内存内容。
    *   它需要两个参数：长度 `N` 和起始地址 `EXPR` (例如 `x 16 0x100000`)。
 `expr()` 函数计算表达式的值,并打印结果。
 `w` (watchpoint) 命令，用于设置一个监视点。
 `d` (delete) 命令，用于删除一个监视点
*/

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
	cpu_exec(n);
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
	char*arg=strtok(args," ");     //使用 strtok 分割参数字符串
	if(arg==NULL){
		printf("Usage: x [length] [address]\n");
		return 0;
	}

	int len=atoi(arg);

	//在上次的字符串上继续查找下一个分隔符
	arg=strtok(NULL," ");
	if(arg==NULL){
		 printf("Usage: x [length] [address]\n");
		 return 0;
	}

	uint32_t addr;
    sscanf(arg, "%x", &addr);
	int i;
	for(i=0;i<len;i++){
		uint32_t data=hwaddr_read(addr+i*4,4);
		printf("0x%08x: 0x%08x\n", addr + i*4, data);
	}
	return 0;
}

static int cmd_w(char *args) {
    if (args == NULL) {
        printf("Usage: w EXPR\n");
        return 0;
    }

    WP *wp = new_wp(args);
    if (wp == NULL) {
        printf("Failed to create new watchpoint.\n");
        return 0;
    }
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
	{ "p", "Evaluate an expression (alias for expr)", cmd_expr },
	{ "w", "Set a watchpoint. Usage: w EXPR", cmd_w },
    { "d", "Delete a watchpoint. Usage: d N", cmd_d },


	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	//继续从主循环已部分解析的字符串中获取 "help" 后面的参数
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		// 遍历整个 cmd_table,打印所有命令的名字和描述
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		//遍历 cmd_table，查找匹配的命令
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