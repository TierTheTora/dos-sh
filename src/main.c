#include "headers/print.h"
#include "headers/dos_exec.h"
#include "headers/parse_opt.h"
#include "headers/dos_cmds.h"
#include "headers/dos_lib.h"
#include "headers/main.h"

#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

struct opt args;
int tickcount;
bool progend;
pthread_t tickthread;

void *
tick (void *arg)
{
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

		usleep(50000);
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

	restore_term();
}

int
main ()
{
	int rc;
	char *line, *path;
	tickcount = 0;
	progend = false;
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
