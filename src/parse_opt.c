#include "headers/parse_opt.h"
#include "headers/dos_cmds.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

char *
get_var_val (const char *varname)
{
	size_t i;
	static char random[7];

	if (strcasecmp(varname, "random") == 0) {
		snprintf(	random,
				sizeof(random),
				"%d",
				rand() % INT16_MAX
		);

		return random;
	}
	for (i = 0; i < vars_cnt; i++) {
		if (strcasecmp(vars[i].name, varname) == 0)
			return vars[i].value;
	}
	return "";
}

const char *
substitute_var (const char *p, char **dest)
{
	char varname[256], *varval;
	int varlen = 0;
	p++;

	while (*p != 0 && *p != '%' && varlen < 256)
		varname[varlen++] = *p++;

	varname[varlen] = 0;

	if (*p != '%')
		return NULL;

	p++;
	varval = get_var_val(varname);

	strcpy(*dest, varval);

	*dest += strlen(varval);

	return p;
}

struct opt
parse_cmd (const char *s)
{
	char *tok, **argv, *dest, **tmp, *trimmed;
	const char *p, *newp;
	int argc, cap, toklen;
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

			while (*p != 0 && *p != '"') {
				if (*p == '%') {
					newp = substitute_var(p, &dest);

					if (newp == NULL) {
						while (*p != 0 && !isspace(*p))
							*dest++ = *p++;
					}
					else
						p = newp;
				}
				else *dest++ = *p++;
			}
			if (*p == '"')
				p++;
		}
		else if (*p == '%') {
			newp = substitute_var(p, &dest);

			if (newp == NULL) {
				while (*p != 0 && !isspace(*p))
					*dest++ = *p++;
			}
			else
				p = newp;
		}
		else {
			while (*p != 0 && !isspace(*p))
				*dest++ = *p++;
		}

		*dest = 0;
		toklen = dest - tok;
		trimmed = malloc(toklen + 1);

		if (trimmed != NULL) {
			strcpy(trimmed, tok);
			free(tok);

			tok = trimmed;
		}

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
