#ifndef MAIN_H
# define MAIN_H

#include "print.h"
#include "dos_lib.h"

#include <locale.h>
#include <readline/readline.h>

#define YEAR (__DATE__ + 7)

#define SRC_LINK	"<https://github.com/TierTheTora/dos-sh.git>"
#define AUTHOR		"TierTheTora"
#define STARTUP_MSG	"\n\tCopyright (c) %s " AUTHOR "\n\n"

#define PRINT_STARTUP_MSG	printf(STARTUP_MSG, YEAR);

/* tps currently only affects cursor blink speed  */
#define DEFAULT_TPS 6

extern struct opt args;
extern int tps, tickcount;
extern memptr_t memsz;
extern bool progend;
extern pthread_t tickthread;

static inline void
init_term ()
{
	rl_catch_signals = 0;
	rl_catch_sigwinch = 0;

	rl_bind_key('\t', rl_insert);
	rl_bind_key('\033', rl_insert);
	/* ctrl+r (reverse-i-search) */
	rl_bind_key('\022', NULL);
	print("\033[3 q");
	setlocale(LC_ALL, "");
}

static inline void
restore_term ()
{
	print("\033[0m");
}

void *tick
	(void *arg);
void kill_dos
	();
void print_help
	();

#endif /* MAIN_H */
