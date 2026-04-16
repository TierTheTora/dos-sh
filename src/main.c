#include "headers/dos_const.h"
#include "headers/conio.h"
#include "headers/print.h"
#include "headers/dos_exec.h"
#include "headers/parse_opt.h"
#include "headers/dos_cmds.h"
#include "headers/dos_lib.h"
#include "headers/main.h"

#include <bits/getopt_core.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <pthread.h>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>

struct opt args;
int tickcount, tps = DEFAULT_TPS;
memptr_t memsz = MEM_MAX;
bool progend, cur_blink = true;
bool color_clear = false;
pthread_t tickthread;

extern char *__progname;

void *
tick (void *arg)
{
	char on;
	on = 'h';

	(void)arg;

	while (true) {
		if (progend) {
			break;
		}

		tickcount++;

		if (cur_blink) {
			printf("\033[?25%c", on);
			fflush(stdout);

			if (on == 'h') on = 'l';
			else on = 'h';
		}

		usleep(1000000 / tps);
	}
	return NULL;
}

void
kill_dos ()
{
	int i;
	progend = true;

	pthread_join(tickthread, NULL);
	clear_history();

	if (args.argv != NULL) {
		for (i = 0; i < args.argc; i++)
			free(args.argv[i]);
	}
	if (memory_freeable)  free(MEMORY);
	if (handles_freeable) free(handles);

	print("\033[0 q");
	print("\033[?25h");
	restore_term();
}

void
print_help ()
{
	printf("Usage: %s [OPTION]\n"
	       "DOS-like shell capable of running COM and BAT files.\n"
	       "\n"
	       "\t-c,\t\t--cursor-blink\tturn off manual cursor blinking\n"
	       "\t-C,\t\t--color-clear\tclear the screen before using the color command\n"
	       "\t-h,\t\t--help\t\tshow this help\n"
	       "\t-m[NUM],\t--mem=[NUM]\tset dos memory\n"
	       "\t-t[NUM],\t--tps=[NUM]\tset ticks per second\n"
	       "\n"
	       "Download source at: " SRC_LINK "\n"
	       "Written by " AUTHOR "\n"
	, __progname);
}

int
main (int argc, char **argv)
{
	int rc, opt;
	char *line, *path;
	tickcount = 0;
	progend = false;
	struct option longopts[] = {
		{ "cursor-blink",	no_argument, NULL, 'c' },
		{ "color-clear",	no_argument, NULL, 'C' },
		{ "help",		no_argument, NULL, 'h' },
		{ "mem" ,		required_argument, NULL, 'm' },
		{ "tps" ,		required_argument, NULL, 't' },
	};

	while ((opt = getopt_long(argc, argv, "cChm:t:", longopts, NULL))
	      != -1) {
		switch (opt) {
		case 'c':
			cur_blink = false;
			break;
		case 'C':
			color_clear = true;
			break;
		case 'h':
			print_help();
			return 0;
		case 'm':
			memsz = atoll(optarg);
			if (memsz <= PRG_START) {
				printf("Memory must have more than %d"
				       " bytes\n", PRG_START);
				return 1;
			}
			break;
		case 't':
			tps = atoi(optarg);
			if (tps <= 0) {
				puts("TPS must be greater than 0.");
				return 1;
			}
			break;
		case '?':
			return 1;
		}
	}

	rc = pthread_create(&tickthread, NULL, &tick, NULL);

	if (rc != 0) {
		perror("pthread_create");
		return 1;
	}

	clrscr();

	if (init_dos() != 0) return 1;

	init_vars();
	init_term();
	atexit(kill_dos);
	print_box("Welcome to DOS in the linux terminal!\n"
	          "Use \"HELP\" for help.");
	PRINT_STARTUP_MSG;
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	exec_noext(	"autoexec",
			(const char*[]){ ".bat" },
			1,
			X_EXEC_SILENT
	);

	for (;;) {
		if (echo) {
			path = get_path();

			if (path == NULL)
				continue;

			line = readline(path);
		}
		else
			line = readline("");
		if (line == NULL) {
			perror("malloc");
			continue;
		}
		if (strlen(line) >= 1) {
			add_history(line);

			args = parse_cmd(line);

			dos_exec(args.argv[0], &args.argv[1],
			         args.argc - 1, false);
		}
	}

	return 0;
}
