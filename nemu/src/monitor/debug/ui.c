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
	uint32_t n = args ? (uint32_t)(atoi(args)) : 1;
	cpu_exec(n);
	return 0;
}

static int cmd_info(char *args) {
	if(*args == 'r'){
		printf("%-16s %-16p %-16x\n", "eax", &cpu.gpr[0]._32, cpu.gpr[0]._32);
		printf("%-16s %-16p %-16x\n", "ecx", &cpu.gpr[1]._32, cpu.gpr[1]._32);
		printf("%-16s %-16p %-16x\n", "edx", &cpu.gpr[2]._32, cpu.gpr[2]._32);
		printf("%-16s %-16p %-16x\n", "ebx", &cpu.gpr[3]._32, cpu.gpr[3]._32);
		printf("%-16s %-16p %-16x\n", "esp", &cpu.gpr[4]._32, cpu.gpr[4]._32);
		printf("%-16s %-16p %-16x\n", "ebp", &cpu.gpr[5]._32, cpu.gpr[5]._32);
		printf("%-16s %-16p %-16x\n", "esi", &cpu.gpr[6]._32, cpu.gpr[6]._32);
		printf("%-16s %-16p %-16x\n", "edi", &cpu.gpr[7]._32, cpu.gpr[7]._32);
		printf("%-16s %-16p %-16x\n", "ax", &cpu.gpr[0]._16, cpu.gpr[0]._16);
		printf("%-16s %-16p %-16x\n", "cx", &cpu.gpr[1]._16, cpu.gpr[1]._16);
		printf("%-16s %-16p %-16x\n", "dx", &cpu.gpr[2]._16, cpu.gpr[2]._16);
		printf("%-16s %-16p %-16x\n", "bx", &cpu.gpr[3]._16, cpu.gpr[3]._16);
		printf("%-16s %-16p %-16x\n", "sp", &cpu.gpr[4]._16, cpu.gpr[4]._16);
		printf("%-16s %-16p %-16x\n", "bp", &cpu.gpr[5]._16, cpu.gpr[5]._16);
		printf("%-16s %-16p %-16x\n", "si", &cpu.gpr[6]._16, cpu.gpr[6]._16);
		printf("%-16s %-16p %-16x\n", "di", &cpu.gpr[7]._16, cpu.gpr[7]._16);
		printf("%-16s %-16p %-16x\n", "ah", &cpu.gpr[0]._8[0], cpu.gpr[0]._8[0]);
		printf("%-16s %-16p %-16x\n", "al", &cpu.gpr[0]._8[1], cpu.gpr[0]._8[1]);
		printf("%-16s %-16p %-16x\n", "ch", &cpu.gpr[1]._8[0], cpu.gpr[1]._8[0]);
		printf("%-16s %-16p %-16x\n", "cl", &cpu.gpr[1]._8[1], cpu.gpr[1]._8[1]);
		printf("%-16s %-16p %-16x\n", "dh", &cpu.gpr[2]._8[0], cpu.gpr[2]._8[0]);
		printf("%-16s %-16p %-16x\n", "dl", &cpu.gpr[2]._8[1], cpu.gpr[3]._8[1]);
		printf("%-16s %-16p %-16x\n", "bh", &cpu.gpr[3]._8[0], cpu.gpr[3]._8[0]);
		printf("%-16s %-16p %-16x\n", "bl", &cpu.gpr[4]._8[1], cpu.gpr[3]._8[1]);
	}
	return 0;
}

static int cmd_x(char *args){
	char delim[] = " ";
	char* token = strtok(args, delim);
	uint32_t n = (uint32_t)atoi(token);
	token = strtok(NULL, delim);
	swaddr_t addr = (swaddr_t)strtol(args, NULL, 0);
	printf("%-#x: ", addr);
	int i = 0;
	for(; i < n; i++)
		printf("%-#x ", swaddr_read(addr + 4 * i, 4));
	printf("\n");
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
	{ "si", "Execute n steps and pause, defalt n = 1", cmd_si},
	{ "info", "-r print register status / -w print watchpoint information", cmd_info},
	{ "x", "Scan memory, output consecutive N 4 bytes in hexadecimal form", cmd_x}
	
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
