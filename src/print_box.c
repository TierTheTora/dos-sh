#include "print_box.h"
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
