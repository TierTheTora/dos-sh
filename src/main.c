#include "headers/print.h"
#include "headers/dos_exec.h"
#include "headers/parse_opt.h"
#include "headers/dos_cmds.h"
#include "headers/dos_lib.h"
#include "headers/main.h"

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
int tickcount, tps = 18;
bool progend;
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

		if (MEMORY != NULL) {
			MEMORY[0x46C] = (BYTE)(tickcount) & 0xFF;
			MEMORY[0x46D] = (BYTE)(tickcount >> 8) & 0xFF;
			MEMORY[0x46E] = (BYTE)(tickcount >> 16) & 0xFF;
			MEMORY[0x46F] = (BYTE)(tickcount >> 24) & 0xFF;
		}

		/* blink cursor */
		printf("\033[?25%c", on);
		fflush(stdout);

		if (on == 'h') on = 'l';
		else on = 'h';

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
	       "\t-h,      --help     \tshow this help\n"
	       "\t-t[NUM], --tps=[NUM]\tset ticks per second\n"
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
		{ "tps" , required_argument, NULL, 't' },
		{ "help", no_argument      , NULL, 'h' },
	};

	while ((opt = getopt_long(argc, argv, "t:h", longopts, NULL))
	      != -1) {
		switch (opt) {
		case 't':
			tps = atoi(optarg);
			if (tps <= 0) {
				puts("TPS must be greater than 0.");
				return 1;
			}
			break;
		case 'h':
			print_help();
			return 0;
		case '?':
			return 1;
		}
	}

	rc = pthread_create(&tickthread, NULL, &tick, NULL);

	if (rc != 0) {
		perror("pthread_create");
		return 1;
	}

	dos_cls();

	if (init_dos() != 0) return 1;

	init_term();
	atexit(kill_dos);
	print_box("Welcome to DOS in the linux terminal!\n"
	          "Use \"HELP\" for help.");
	print(STARTUP_MSG);
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

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
