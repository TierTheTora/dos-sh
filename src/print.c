#include "headers/print.h"
#include "headers/conio.h"
#include "headers/keyb.h"

#include <linux/limits.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

void
put_tabl_h (size_t len)
{
	size_t i;

	putchar('+');

	for (i = 0; i < len; i++) {
		putchar('-');
	}

	puts("+");
}

void
put_tabl_v (const char *msg, size_t len, struct substr_info ssi)
{
	size_t i, j, padding, ch;
	ch = 0;

	putchar('|');

	for (i = 0; i <= len; i++, ch++) {
		if (msg[i] == '\n'
		 || i == len) {
			padding = ssi.longest_substr - ch;

			for (j = 0; j < padding; j++) {
				putchar(' ');
			}

			if (i != len) {
				puts("|");
				putchar('|');
			}

			ch = -1;
		}
		else putchar(msg[i]);
	}

	puts("|");
}

struct substr_info
get_longest_substr (const char *s, char delim)
{
	size_t i, slen;
	struct substr_info ssi;
	slen = 0;
	ssi.delim_cnt = 0;
	ssi.longest_substr = 0;

	for (i = 0; s[i] != 0; i++) {
		if (s[i] == delim) {
			if (ssi.longest_substr < slen)
				ssi.longest_substr = slen;
			ssi.delim_cnt++;
			slen = ssi.delim_cnt;
		}
		slen++;
	}
	if (ssi.longest_substr < slen)
		ssi.longest_substr = slen;

	return ssi;
}

void
print_box (const char *msg)
{
	int msglen;
	struct substr_info ssi;
	msglen = strlen(msg);
	ssi = get_longest_substr(msg, '\n');

	put_tabl_h(ssi.longest_substr);
	put_tabl_v(msg, msglen, ssi);
	put_tabl_h(ssi.longest_substr);
}

void
dosify_dir (char *path)
{
	int i;

	for (i = 0; path[i] != 0; i++) {
		if (path[i] == '/')
			path[i] = '\\';
		else if (path[i] == '\\')
			path[i] = '/';
	}
}

void
undosify_dir (char *path)
{
	int i;

	for (i = 0; path[i] != 0; i++) {
		if (path[i] == '\\')
			path[i] = '/';
		else if (path[i] == '/')
			path[i] = '\\';
	}
}

void
print_path ()
{
	char path[PATH_MAX + 1];

	memset(path, 0, sizeof(path));

	if (getcwd(path, sizeof(path)) != NULL) {
		dosify_dir(path);
		write(STDIN_FILENO, path, sizeof(path));
		putchar('>');
		fflush(stdout);
	}
	else {
		perror("getcwd");
		abort();
	}
}

void
print_readable_bytes (size_t bytes)
{
	double pb, tb, gb, mb, kb;

	pb = (double)bytes / PBYTE_SIZE;
	tb = (double)bytes / TBYTE_SIZE;
	gb = (double)bytes / GBYTE_SIZE;
	mb = (double)bytes / MBYTE_SIZE;
	kb = (double)bytes / KBYTE_SIZE;

	if (pb >= 1.0f) {
		printf("%20.1fPb\n", pb);
		return;
	}
	else if (tb >= 1.0f) {
		printf("%20.1fTb\n", tb);
		return;
	}
	else if (gb >= 1.0f) {
		printf("%20.1fGb\n", gb);
		return;
	}
	else if (mb >= 1.0f) {
		printf("%20.1fMb\n", mb);
		return;
	}
	else if (kb >= 1.0f) {
		printf("%20.1fKb\n", kb);
		return;
	}
	else
		printf("%20zub\n", bytes);
}

int
readprompt (char **buffer, int *bytes, bool *buf_freeable)
{
	int chptr, bytes_read, move_back;
	char ch, seq1, seq2;
	chptr = bytes_read = 0;

	while (true) {
		ch = getch();

		if (ch == EOF) return -1;
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
		if (ch == '\t')
			ch = ' ';
		if (ch == K_BACKSP) {
			if (chptr > 0) {
				memmove(&(*buffer)[chptr - 1],
			                &(*buffer)[chptr],
			                bytes_read - chptr);
				
				bytes_read--;
				chptr--;
				(*buffer)[bytes_read] = 0;

				move_back = bytes_read - chptr + 1;

				putchar('\b');
				putchar(' ');

				if (move_back > 0)
					printf("\033[%dD", move_back);

				if (bytes_read - chptr > 0) {
					write(STDOUT_FILENO,
					      &(*buffer)[chptr],
					      bytes_read - chptr);
				}

			}
			continue;
		}
		if (bytes_read + 2 >= (*bytes)) {
			(*bytes) *= 2;
			char *tmp = realloc((*buffer),
				(*bytes) * sizeof(char));

			if (tmp == NULL) {
				(*buf_freeable) = false;
				perror("realloc");
				return -1;
			}

			(*buffer) = tmp;
		}
		if (ch == '\n') {
			putchar('\n');
			break;
		}

		memmove(&(*buffer)[chptr + 1],
		        &(*buffer)[chptr],
		        bytes_read - chptr);

		(*buffer)[chptr] = (char)ch;
		chptr++;
		bytes_read++;
		(*buffer)[bytes_read] = 0;

		putchar(ch);
		if (bytes_read - chptr > 0) {
			printf("%*s", bytes_read - chptr, &(*buffer)[chptr]);
			printf("\033[%dD", bytes_read - chptr);
		}
	}

	return bytes_read;
}

int
dos_read (char *buffer, size_t max)
{
	int chptr, bytes_read, move_back;
	char ch, seq1, seq2;
	chptr = bytes_read = 0;

	while (true) {
		ch = getch();

		if (ch == EOF) return -1;
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
		if (ch == '\t')
			ch = ' ';
		if (ch == K_BACKSP) {
			if (chptr > 0) {
				memmove(&buffer[chptr - 1],
			                &buffer[chptr],
			                bytes_read - chptr);
				
				bytes_read--;
				chptr--;
				buffer[bytes_read] = 0;

				putchar('\b');
				if (bytes_read - chptr > 0) {
					write(STDOUT_FILENO,
					      &buffer[chptr],
					      bytes_read - chptr);
				}

				putchar(' ');

				move_back = bytes_read - chptr + 1;

				if (move_back > 0) {
					printf("\033[%dD", move_back);
				}
			}
			continue;
		}
		if ((size_t)(bytes_read) + 2 >= max) {
			while ((ch = getch()) != '\n');
			return 0;
		}
		if (ch == '\n')
			break;

		memmove(&buffer[chptr + 1],
		        &buffer[chptr],
		        bytes_read - chptr);

		buffer[chptr] = (char)ch;
		chptr++;
		bytes_read++;
		buffer[bytes_read] = 0;

		putchar(ch);
		if (bytes_read - chptr > 0) {
			printf("%*s", bytes_read - chptr, &buffer[chptr]);
			printf("\033[%dD", bytes_read - chptr);
		}
	}

	return bytes_read;
}
