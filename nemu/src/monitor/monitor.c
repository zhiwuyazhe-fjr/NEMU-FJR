#include "nemu.h"
#include "memory/cache.h"
#include "memory/tlb.h"

#define ENTRY_START 0x100000

extern uint8_t entry [];
extern uint32_t entry_len;
extern char *exec_file;

void load_elf_tables(int, char *[]);
void init_regex();
void init_wp_pool();
void init_ddr3();
void init_cache();
void init_tlb();

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
}

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

	/* The initial value of EFLAGS after reset (i386 manual, chapter 10). */
	cpu.eflags.val = 0x0002;

	/* Enter the real mode on reset: segmentation and paging are off. */
	cpu.cr0.val = 0;
	cpu.cr3.val = 0;
	cpu.gdtr.base = 0;
	cpu.gdtr.limit = 0;

	/* IA-32 forbids loading CS with mov, but instruction fetching needs
	 * CS right after CR0.PE is set.  Initialize the descriptor cache of
	 * every segment register as a flat mapping (base = 0, limit = 4G),
	 * so that setting PE alone does not change any address. */
	{
		int i;
		for(i = R_ES; i < NR_SREG; i ++) {
			cpu.sreg[i].sel = 0;
			cpu.sreg[i].base = 0;
			cpu.sreg[i].limit = 0xffffffff;
		}
	}

	/* Initialize DRAM, the cache and the TLB. */
	init_ddr3();
	init_cache();
	init_tlb();
}
