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

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },

	{ "si", "Step N instructions and pause (default N=1), e.g. si 10", cmd_si },
	{ "info", "Print program state: info r (registers) / info w (watchpoints)", cmd_info },
	{ "x", "Scan memory: x N EXPR, e.g. x 10 0x100000", cmd_x },
	{ "p", "Evaluate an expression, e.g. p 4 + 3 * (2 - 1)", cmd_p },

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

static int cmd_si(char *args) {
	int n = 1;
	if (args != NULL) {
		n = atoi(args);
		if (n <= 0) {
			printf("Usage: si [N], N must be a positive integer\n");
			return 0;
		}
	}
	cpu_exec(n);
	return 0;
}

static int cmd_info(char *args) {
	char *subcmd = args == NULL ? NULL : strtok(args, " ");

	if (subcmd == NULL) {
		printf("Usage: info r | info w\n");
	}
	else if (strcmp(subcmd, "r") == 0) {
		printf("%-4s 0x%08x\t%d\n", "eax", cpu.eax, cpu.eax);
		printf("%-4s 0x%08x\t%d\n", "ecx", cpu.ecx, cpu.ecx);
		printf("%-4s 0x%08x\t%d\n", "edx", cpu.edx, cpu.edx);
		printf("%-4s 0x%08x\t%d\n", "ebx", cpu.ebx, cpu.ebx);
		printf("%-4s 0x%08x\t0x%x\n", "esp", cpu.esp, cpu.esp);
		printf("%-4s 0x%08x\t0x%x\n", "ebp", cpu.ebp, cpu.ebp);
		printf("%-4s 0x%08x\t%d\n", "esi", cpu.esi, cpu.esi);
		printf("%-4s 0x%08x\t%d\n", "edi", cpu.edi, cpu.edi);
		printf("%-4s 0x%08x\t0x%x\n", "eip", cpu.eip, cpu.eip);
	}
	else if (strcmp(subcmd, "w") == 0) {
		/* TODO: print watchpoints once they are implemented (PA1 stage 3). */
		printf("No watchpoints.\n");
	}
	else {
		printf("Unknown info subcommand '%s'\n", subcmd);
	}
	return 0;
}

static int cmd_x(char *args) {
	if (args == NULL) {
		printf("Usage: x N EXPR, e.g. x 10 0x100000 or x 10 $esp\n");
		return 0;
	}

	/* Split off the first token as N; everything after it is the expression,
	 * which may itself contain spaces (e.g. "x 10 $esp + 4"). This mirrors
	 * how ui_mainloop() splits the command from its arguments. */
	char *args_end = args + strlen(args);
	char *n_str = strtok(args, " ");
	char *expr_str = n_str == NULL ? NULL : n_str + strlen(n_str) + 1;

	if (n_str == NULL || expr_str == NULL || expr_str >= args_end) {
		printf("Usage: x N EXPR, e.g. x 10 0x100000 or x 10 $esp\n");
		return 0;
	}

	int n = atoi(n_str);

	bool success = true;
	swaddr_t addr = expr(expr_str, &success);
	if (!success) {
		printf("Invalid expression '%s'\n", expr_str);
		return 0;
	}

	int i;
	for (i = 0; i < n; i ++) {
		if (i % 4 == 0) {
			if (i != 0) { printf("\n"); }
			printf("0x%08x:", addr + i * 4);
		}
		uint32_t data = swaddr_read(addr + i * 4, 4);
		printf("  0x%08x", data);
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args) {
	if (args == NULL) {
		printf("Usage: p EXPR\n");
		return 0;
	}

	bool success = true;
	uint32_t val = expr(args, &success);
	if (!success) {
		printf("Invalid expression '%s'\n", args);
	}
	else {
		printf("%u (0x%08x)\n", val, val);
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
