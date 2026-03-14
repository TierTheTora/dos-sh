#include "parse_opt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

struct opt
parse_cmd (const char *s)
{
	char *tok, **argv, *dest, **tmp;
	const char *p;
	int argc, cap;
	struct opt ret;

	p = s;
	argc = 0;
	cap = 16;
	argv = malloc(cap * sizeof(char *));

	if (argv == NULL) {
		perror("malloc");
		ret.argv = NULL;
		ret.argc = 0;
		return ret;
	}

	while (*p != 0) {
		while (isspace(*p))
			p++;
		if (*p == 0)
			break;

		tok = malloc((strlen(p) + 1) * sizeof(char));

		if (tok == NULL) {
			perror("malloc");
			for (int i = 0; i < argc; i++)
				free(argv[i]);
			ret.argv = NULL;
			ret.argc = 0;
			return ret;
		}

		dest = tok;

		if (*p == '"') {
			p++;
			while (*p != 0 && *p != '"')
				*dest++ = *p++;
			if (*p == '"')
				p++;
		}
		else {
			while (*p != 0 && !isspace(*p))
				*dest++ = *p++;
		}

		*dest = 0;

		if (argc >= cap - 1) {
			cap *= 2;
			tmp = realloc(argv, cap * sizeof(char *));
			if (tmp == NULL) {
				perror("realloc");
				for (int i = 0; i < argc; i++)
					free(argv[i]);
				free(tok);
				ret.argv = NULL;
				ret.argc = 0;
				return ret;
			}
			argv = tmp;
		}
		argv[argc++] = tok;
	}

	argv[argc] = NULL;
	ret.argv = argv;
	ret.argc = argc;

	return ret;
}
