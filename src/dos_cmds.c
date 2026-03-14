#include "dos_cmds.h"
#include "dos_const.h"
#include "dos_lib.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

void
dos_exit ()
{
	exit(0);
}

void
dos_help (char **argv, int argc)
{
	int i;
	bool all;

	all = false;

	puts("<EXIT > Exit from the shell\n"
	     "<HELP > Show this help\n"
	     "<VER  > Show the DOS version\n"
	     "<ECHO > Display text to screen"
	);

	for (i = 0; i < argc; i++) {
		if (strcasecmp(argv[i], "/all") == 0) {
			all = true;
		}
	}
	if (all == true)
		puts("<PAUSE> Wait until keystroke\n"
		     "<TYPE > Display the contents of a text file\n"
		     "<CLS  > Clear the screen");
}

void
dos_ver ()
{
	puts("DOS version " DOS_VERSION);
}

void
dos_echo (char **argv, int argc)
{
	int i, arglen;

	for (i = 0; i < argc; i++) {
		arglen = strlen(argv[i]);

		write(STDOUT_FILENO, argv[i], arglen);
		putchar(' ');
		fflush(stdout);
	}

	putchar('\n');
}

//void
//dos_dir ()
//{
//
//}

void
dos_pause ()
{
	puts("Press any key to continue . . .\n");
	(void)getch();
}

void
dos_type (char **argv, int argc)
{
	int i, fd;
	char c;

	if (argc == 0) {
		puts("The syntax of the command is incorrect.");
		return;
	}
	for (i = 0; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		
		if (fd == -1) {
			printf("The file %s does not exist.",
			       argv[i]);
			continue;
		}

		while (read(fd, &c, 1) == 1) {
			putchar(c);
		}
		
		close(fd);
	}
}

void
dos_cls ()
{
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
}
