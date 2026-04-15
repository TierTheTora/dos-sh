#ifndef DOS_EXEC_H
#define DOS_EXEC_H

#include <stdbool.h>

#define X_EXEC_SILENT	true
#define X_EXEC_VERBOSE	false

int exec_noext
	(const char *cmd, const char *ext[], int ext_cnt, bool err);
void dos_exec
	(const char *cmd, char **argv, int argc, bool isbatfile);

#endif /* DOS_EXEC_H */
