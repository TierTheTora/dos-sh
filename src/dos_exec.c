#include "dos_exec.h"
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>

#include "dos_cmds.h"

void
dos_exec (const char *cmd, char **argv, int argc)
{
	if (cmd == NULL) return;
	if (strcasecmp(cmd, "exit") == 0)
		dos_exit();
	else if (strcasecmp(cmd, "help") == 0)
		dos_help(argv, argc);
	else if (strcasecmp(cmd, "ver") == 0)
		dos_ver();
	else if (strcasecmp(cmd, "echo") == 0)
		dos_echo(argv, argc);
	else if (strcasecmp(cmd, "pause") == 0)
		dos_pause();
	else if (strcasecmp(cmd, "type") == 0)
		dos_type(argv, argc);
	else if (strcasecmp(cmd, "cls") == 0)
		dos_cls();
	else if (strcasecmp(cmd, "cd") == 0)
		dos_cd(argv, argc);
	else if (strcasecmp(cmd, "chdir") == 0)
		dos_cd(argv, argc);
	else if (strcasecmp(cmd, "dir") == 0)
		dos_dir(argv, argc);
	else if (strcasecmp(cmd, "rem") != 0)
		printf("Illegal command: %s.\n", cmd);

	putchar('\n');
}
