#include "headers/print.h"
#include "headers/dos_exec.h"
#include "headers/parse_opt.h"
#include "headers/dos_cmds.h"
#include "headers/conio.h"
#include "headers/keyb.h"

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
	int bytes, bytes_read, chptr;
	char ch, seq1, seq2;
	bytes = 256;
	bytes_read = chptr = 0;
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
		if (echo) print_path();
		chptr = 0;
		bytes_read = 0;
		buffer[0] = 0;

		while (true) {
			ch = getch();

			if (ch == EOF) return 0;
			if (ch == '\033') {
				seq1 = getch();
				seq2 = getch();

				if (seq1 == '[') {
					switch (seq2) {
					case 'C':
						if (chptr < bytes_read) {
							chptr++;
							print("\033[C");
						}
						break;
					case 'D':
						if (chptr > 0) {
							chptr--;
							print("\033[D");
						}
						break;
					}
				}
				continue;
			}

			if (ch == K_BACKSP) {
				if (chptr > 0) {
					memmove(&buffer[chptr - 1],
				        &buffer[chptr],
				        bytes_read - chptr);
					
					bytes_read--;
					chptr--;
					buffer[bytes_read] = 0;

					print("\r\033[K");
					
					if (echo) print_path();
					
					print(buffer);
					int move_back = bytes_read - chptr;
					while (move_back--) printf("\033[D");
				}
				continue;
			}

			if (bytes_read + 2 >= bytes) {
				bytes *= 2;
				char *tmp = realloc(buffer,
					bytes * sizeof(char));

				if (tmp == NULL) {
					buf_freeable = false;
					perror("realloc");
					return 1;
				}

				buffer = tmp;
			}

			if (ch == '\n') {
				putchar('\n');
				break;
			}

			memmove(&buffer[chptr + 1],
			        &buffer[chptr],
			        bytes_read - chptr);

			buffer[chptr] = (char)ch;
			chptr++;
			bytes_read++;
			buffer[bytes_read] = 0;

			print("\r\033[K");
			if (echo) print_path();
			print(buffer);

			int move_back = bytes_read - chptr;
				while (move_back--) printf("\033[D");
		}

		if (bytes_read > 1) {
			args = parse_cmd(buffer);
			dos_exec(args.argv[0], &args.argv[1], args.argc - 1);
		}
	}

	return 0;
}
