#ifndef MAIN_H
# define MAIN_H

#include "print.h"

#include <locale.h>
#include <readline/readline.h>

extern struct opt args;

static inline void
init_term ()
{
	rl_bind_key('\t', rl_insert);
	/* ctrl+r (reverse-i-search) */
	rl_bind_key('\022', NULL);
	setlocale(LC_ALL, "");
}

static inline void
restore_term ()
{
	print("\033[0m");
}

void kill_dos
	();

#endif /* MAIN_H */
