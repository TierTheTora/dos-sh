#ifndef DOS_EXEC_H
#define DOS_EXEC_H

#include <stdbool.h>

int exec_noext
	(const char *cmd, const char *ext[], int ext_cnt);
void dos_exec
	(const char *cmd, char **argv, int argc, bool isbatfile);

#endif /* DOS_EXEC_H */
