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
#include <locale.h>
#include <pthread.h>

char *buffer;
char *tmpbuf;
bool buf_freeable;
struct opt args;
struct termios oldt;
int tickcount;
bool progend;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t tickthread;

int
init_term ()
{
	struct termios newt;

	if (tcgetattr(STDIN_FILENO, &oldt) == -1)
		return -1;

	newt = oldt;
	newt.c_lflag &= (tcflag_t)(~(ICANON | ECHO | ISIG));
	newt.c_cc[VMIN] = 1;
	newt.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1)
		return -1;

	setlocale(LC_ALL, "");

	return 0;
}

void
restore_term ()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	print("\033[0m");
}

void *
tick (void *arg)
{
	(void)arg;

	while (true) {
		pthread_mutex_lock(&lock);

		if (progend) {
			pthread_mutex_unlock(&lock);

			break;
		}
		tickcount++;

		if (MEMORY != NULL) {
			MEMORY[0x46C] = (BYTE)(tickcount) & 0xFF;
			MEMORY[0x46D] = (BYTE)(tickcount >> 8) & 0xFF;
			MEMORY[0x46E] = (BYTE)(tickcount >> 16) & 0xFF;
			MEMORY[0x46F] = (BYTE)(tickcount >> 24) & 0xFF;
		}

		pthread_mutex_unlock(&lock);
		usleep(50000);
	}

	return NULL;
}

void
kill_dos ()
{
	int i;

	pthread_mutex_lock(&lock);

	progend = true;

	pthread_mutex_unlock(&lock);
	pthread_join(tickthread, NULL);

	if (buf_freeable == true)
		free(buffer);
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
	int bytes, bytes_read, rc;
	bytes = 256;
	buffer = malloc((long unsigned int)bytes * sizeof(char));
	tickcount = 0;
	progend = false;
	rc = pthread_create(&tickthread, NULL, &tick, NULL);

	if (rc != 0) {
		perror("pthread_create");
		return 1;
	}

	dos_cls();

	if (buffer == NULL) {
		buf_freeable = false;
		perror("malloc");
		return 1;
	}

	if (init_dos() != 0) return 1;

	buf_freeable = true;

	if (init_term() == -1) {
		puts("Failed to initialize terminal.");
		return -1;
	}

	atexit(kill_dos);
	print_box("Welcome to DOS in the linux terminal!\n"
	          "Use \"HELP\" for help.");

	for (;;) {
		if (echo) print_path();
		buffer[0] = 0;

		bytes_read = readprompt(&buffer, &bytes, &buf_freeable);

		if (bytes_read >= 1) {
			args = parse_cmd(buffer);
			dos_exec(args.argv[0], &args.argv[1],
			         args.argc - 1, false);
		}
	}

	return 0;
}
