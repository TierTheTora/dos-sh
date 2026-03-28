#include "headers/print.h"
#include "headers/dos_exec.h"
#include "headers/parse_opt.h"
#include "headers/dos_cmds.h"

#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

char *buffer;
char *tmpbuf;
bool buf_freeable;
struct opt args;

void
free_all ()
{
	int i;

	if (buf_freeable == true)
		free(buffer);
	if (args.argv != NULL) {
		for (i = 0; i < args.argc; i++)
			free(args.argv[i]);
	}
}

int
main ()
{
	int bytes, bytes_read;
	bytes = 256;
	buffer = malloc(bytes * sizeof(char));

	dos_cls();

	if (buffer == NULL) {
		buf_freeable = false;
		perror("malloc");
		abort();
	}

	buf_freeable = true;

	atexit(free_all);

	print_box("Welcome to DOS in the linux terminal!\n"
	          "Use \"HELP\" for help.");

	
	for (;;) {
		if (echo)
			print_path();

		bytes_read = read(STDIN_FILENO, buffer, bytes);
		if (bytes_read < 0) {
			perror("read");
			abort();
		}
		if (bytes_read < 2)
			continue;
		if (bytes_read >= bytes) {
			bytes = bytes_read + 1;
			tmpbuf = realloc(buffer, bytes * sizeof(char));
			if (tmpbuf == NULL) {
				buf_freeable = false;
				perror("realloc");
				puts("Command too long!");
				continue;
			}
			else {
				buffer = tmpbuf;

			}
		}
		buffer[bytes_read - 1] = 0;
		args = parse_cmd(buffer);

		dos_exec(args.argv[0], &args.argv[1], args.argc - 1);
	}

	return 0;
}
