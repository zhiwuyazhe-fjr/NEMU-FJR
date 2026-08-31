#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
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

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args) {
	int n = 1;
	if(args != NULL) {
		sscanf(args, "%d", &n);
	}
	if(n <= 0) {
		printf("Usage: si [N], N should be positive\n");
		return 0;
	}
	cpu_exec(n);
	return 0;
}

static int cmd_info(char *args) {
	char *arg = strtok(NULL, " ");

	if(arg == NULL) {
		printf("Usage: info r (registers) or info w (watchpoints)\n");
		return 0;
	}

	if(strcmp(arg, "r") == 0) {
		int i;
		for(i = R_EAX; i <= R_EDI; i ++) {
			printf("%s\t0x%08x\t%u\n", regsl[i], reg_l(i), reg_l(i));
		}
		printf("eip\t0x%08x\n", cpu.eip);
	}
	else if(strcmp(arg, "w") == 0) {
		print_wp();
	}
	else {
		printf("Unknown argument '%s'\n", arg);
	}

	return 0;
}

static int cmd_x(char *args) {
	if(args == NULL) {
		printf("Usage: x N EXPR\n");
		return 0;
	}

	char *endp;
	int n = (int)strtoul(args, &endp, 10);
	if(endp == args || n <= 0) {
		printf("Usage: x N EXPR\n");
		return 0;
	}
	while(*endp == ' ') { endp ++; }
	if(*endp == '\0') {
		printf("Usage: x N EXPR\n");
		return 0;
	}

	bool success = false;
	uint32_t base = expr(endp, &success);
	if(!success) {
		printf("bad expression\n");
		return 0;
	}

	printf("0x%08x:", base);
	int i;
	for(i = 0; i < n; i ++) {
		printf("  0x%08x", swaddr_read(base + i * 4, 4));
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args) {
	if(args == NULL) {
		printf("Usage: p EXPR\n");
		return 0;
	}

	bool success = false;
	uint32_t val = expr(args, &success);
	if(!success) {
		printf("bad expression\n");
		return 0;
	}
	printf("0x%08x %u\n", val, val);
	return 0;
}

static int cmd_w(char *args) {
	if(args == NULL) {
		printf("Usage: w EXPR\n");
		return 0;
	}

	bool success = false;
	uint32_t val = expr(args, &success);
	if(!success) {
		printf("bad expression\n");
		return 0;
	}

	WP *wp = new_wp();
	snprintf(wp->expr, sizeof(wp->expr), "%s", args);
	wp->old_val = val;
	printf("Set watchpoint %d: %s = 0x%08x\n", wp->NO, wp->expr, val);
	return 0;
}

static int cmd_d(char *args) {
	if(args == NULL) {
		printf("Usage: d N\n");
		return 0;
	}

	int no;
	if(sscanf(args, "%d", &no) != 1) {
		printf("Usage: d N\n");
		return 0;
	}

	WP *target = find_wp(no);
	if(target == NULL) {
		printf("No watchpoint number %d\n", no);
		return 0;
	}
	free_wp(target);
	return 0;
}

/* walk the stack frame chain through %ebp */
static int cmd_bt(char *args) {
	uint32_t ebp = reg_l(R_EBP);
	char name[64];
	int i;

	for(i = 0; ebp != 0 && i < 32; i ++) {
		uint32_t prev_ebp = swaddr_read(ebp, 4);
		uint32_t ret_addr = swaddr_read(ebp + 4, 4);
		func_name(ret_addr - 1, name);
		printf("#%d 0x%08x in %s (0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", i, ret_addr, name,
				swaddr_read(ebp + 8, 4), swaddr_read(ebp + 12, 4),
				swaddr_read(ebp + 16, 4), swaddr_read(ebp + 20, 4));
		ebp = prev_ebp;
	}
	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Step N instructions, si [N]", cmd_si },
	{ "info", "Print registers (info r) or watchpoints (info w)", cmd_info },
	{ "x", "Scan memory: x N EXPR", cmd_x },
	{ "p", "Evaluate the expression: p EXPR", cmd_p },
	{ "w", "Set a watchpoint: w EXPR", cmd_w },
	{ "d", "Delete a watchpoint: d N", cmd_d },
	{ "bt", "Print the stack frame chain", cmd_bt },

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

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
