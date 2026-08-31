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
		printf("Usage: info r\n");
		return 0;
	}

	if(strcmp(arg, "r") == 0) {
		int i;
		for(i = R_EAX; i <= R_EDI; i ++) {
			printf("%s\t0x%08x\t%u\n", regsl[i], reg_l(i), reg_l(i));
		}
		printf("eip\t0x%08x\n", cpu.eip);
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
	{ "info", "Print registers: info r", cmd_info },
	{ "x", "Scan memory: x N EXPR", cmd_x },
	{ "p", "Evaluate the expression: p EXPR", cmd_p },

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
