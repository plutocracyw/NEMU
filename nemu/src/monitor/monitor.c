#include "nemu.h"
#include "memory/cache.h"

#define ENTRY_START 0x100000

extern uint8_t entry [];
extern uint32_t entry_len;
extern char *exec_file;

void load_elf_tables(int, char *[]);
void init_regex();
void init_wp_pool();
void init_ddr3();

FILE *log_fp = NULL;

static void init_log() {
	log_fp = fopen("log.txt", "w");
	Assert(log_fp, "Can not open 'log.txt'");
}

static void welcome() {
	printf("Welcome to NEMU!\nThe executable is %s.\nFor help, type \"help\"\n",
			exec_file);
}

void init_monitor(int argc, char *argv[]) {
	/* Perform some global initialization */

	/* Open the log file. */
	init_log();

	/* Load the string table and symbol table from the ELF file for future use. */
	load_elf_tables(argc, argv);

	/* Compile the regular expressions. */
	init_regex();

	/* Initialize the watchpoint pool. */
	init_wp_pool();

	/* Display welcome message. */
	welcome();
}

#ifdef USE_RAMDISK
static void init_ramdisk() {
	int ret;
	const int ramdisk_max_size = 0xa0000;
	FILE *fp = fopen(exec_file, "rb");
	Assert(fp, "Can not open '%s'", exec_file);

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	Assert(file_size < ramdisk_max_size, "file size(%zd) too large", file_size);

	fseek(fp, 0, SEEK_SET);
	ret = fread(hwa_to_va(0), file_size, 1, fp);
	assert(ret == 1);
	fclose(fp);
}
#endif

static void load_entry() {
	int ret;
	FILE *fp = fopen("entry", "rb");
	Assert(fp, "Can not open 'entry'");

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);

	fseek(fp, 0, SEEK_SET);
	ret = fread(hwa_to_va(ENTRY_START), file_size, 1, fp);
	assert(ret == 1);
	fclose(fp);
}  // restart


void restart() {
    /* Perform some initialization to restart a program */
#ifdef USE_RAMDISK
    /* Read the file with name `argv[1]' into ramdisk. */
    init_ramdisk();
#endif

    /* Read the entry code into memory. */
    load_entry();

    /* Set the initial instruction pointer. */
    cpu.eip = ENTRY_START;

    /* Initialize DRAM. */
    init_ddr3();
    init_cache();

    /* ====== IA-32 分段机制相关寄存器初始化 ====== */
    
    /* 复位通用寄存器 */
    cpu.eax = 0;
    cpu.ecx = 0; 
    cpu.edx = 0;
    cpu.ebx = 0;
    cpu.esp = 0x7C00;  // 典型实模式栈地址
    cpu.ebp = 0;
    cpu.esi = 0;
    cpu.edi = 0;

    /* 复位标志寄存器 */
    cpu.eflags.val = 0x00000002;  // 位1保留为1

    /* 初始化控制寄存器 CR0 - 实模式 */
    cpu.CR0.val = 0x00000000;
    cpu.CR0.PE = 0;  // 实模式

    /* 初始化系统表寄存器 */
    cpu.GDTR.base = 0;
    cpu.GDTR.limit = 0;
    cpu.IDTR.base = 0;
    cpu.IDTR.limit = 0x3FF;

    /* 初始化所有段寄存器 - 实模式 */
    // CS - 代码段
    cpu.CS.selector = 0;
    cpu.CS.base = 0;
    cpu.CS.limit = 0xFFFF;    // 实模式是64KB
    cpu.CS.present = 1;

    // DS - 数据段
    cpu.DS.selector = 0;
    cpu.DS.base = 0;
    cpu.DS.limit = 0xFFFF;
    cpu.DS.present = 1;

    // ES - 附加段
    cpu.ES.selector = 0;
    cpu.ES.base = 0;
    cpu.ES.limit = 0xFFFF;
    cpu.ES.present = 1;

    // SS - 堆栈段
    cpu.SS.selector = 0;
    cpu.SS.base = 0;
    cpu.SS.limit = 0xFFFF;
    cpu.SS.present = 1;

    // FS - 附加段
    cpu.FS.selector = 0;
    cpu.FS.base = 0;
    cpu.FS.limit = 0xFFFF;
    cpu.FS.present = 1;

    // GS - 附加段
    cpu.GS.selector = 0;
    cpu.GS.base = 0;
    cpu.GS.limit = 0xFFFF;
    cpu.GS.present = 1;

    printf("CPU restarted in Real Mode\n");
}