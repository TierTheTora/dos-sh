#include "headers/dos_exec.h"
#include "headers/main.h"
#include "headers/conio.h"
#include "headers/dos_lib.h"
#include "headers/print.h"
#include "headers/dos_cmds.h"

#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdlib.h>

int
exec_noext (const char *cmd, const char *ext[], int ext_cnt)
{
	int fd, i;
	REGS r;
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	char *fext;
	char cmd2[strlen(cmd) + 10], *path, *last_slash,
	     *base_cmd, name[strlen(cmd) + 10];

	undosify_dir((char *)cmd);
	strcpy(cmd2, cmd);

	path = strdup(cmd);
	last_slash = strrchr(path, '/');

	if (last_slash != NULL) {
		*last_slash = 0;
		base_cmd = last_slash + 1;
	}
	else {
		strcpy(path, ".");
		base_cmd = (char *)cmd;
	}
	
	fd = -1;
	dir = opendir(path);;

	if (dir) {
		while ((ent = readdir(dir)) != NULL) {
			for (i = 0; i < ext_cnt; i++) {
				snprintf(name, sizeof(name),
					 "%s%s",
					 base_cmd, ext[i]);

				if (strcasecmp(ent->d_name,
					       name) == 0) {
					snprintf(cmd2, sizeof(cmd2),
						 "%s/%s",
						 path, ent->d_name);

					fd = open(cmd2, O_RDONLY);

					if (fd != -1) break;
				}
			}
			if (fd != -1) break;
		}
		closedir(dir);
	}
	if (fd == -1) {
		illegal:
		dosify_dir((char *)cmd);
		printf("Illegal command: %s.\n", cmd);
		return -1;
	}

	fext = strrchr(cmd2, '.');
	if (fext && strcasecmp(fext, ".com") == 0) {
		if (lstat(cmd2, &statbuf) == -1) {
			perror("lstat");

			return -1;
		}

		runcom(&r, fd, statbuf.st_size);
	}
	else if (fext && strcasecmp(fext, ".bat") == 0)
		runbat(fd);
	else goto illegal;

	close(fd);
	free(path);

	return 0;
}

void
dos_exec (const char *cmd, char **argv, int argc, bool isbatfile)
{
	const char *ext[] = { ".com", ".bat", "" };
	int ext_cnt = 3;
	
	if (cmd == NULL) return;
	if (*cmd == '@') {
		cmd++;
		if (*cmd == 0) return;
	}

	if (strcasecmp(cmd, "box") == 0)
		dos_box(argv, argc);

	else if (strcasecmp(cmd, "call") == 0)
		dos_call(argv, argc);

	else if (strcasecmp(cmd, "cd") == 0)
		dos_cd(argv, argc);
	else if (strcasecmp(cmd, "chdir") == 0)
		dos_cd(argv, argc);

	else if (strcasecmp(cmd, "cls") == 0) {
		dos_cls();
		return;
	}

	else if (strcasecmp(cmd, "color") == 0) {
		dos_color(argv, argc);
		if (color_clear == true) {
			clrscr();
			return;
		}
	}

	else if (strcasecmp(cmd, "copy") == 0)
		dos_copy(argv, argc);

	else if (strcasecmp(cmd, "del") == 0)
		dos_del(argv, argc);
	else if (strcasecmp(cmd, "delete") == 0)
		dos_del(argv, argc);

	else if (strcasecmp(cmd, "dir") == 0)
		dos_dir(argv, argc);

	else if (strcasecmp(cmd, "erase") == 0)
		dos_del(argv, argc);

	else if (strcasecmp(cmd, "echo") == 0)
		dos_echo(argv, argc);

	else if (strcasecmp(cmd, "exit") == 0)
		dos_exit();

	else if (strcasecmp(cmd, "fc") == 0)
		dos_fc(argv, argc);

	else if (strcasecmp(cmd, "free") == 0)
		dos_free(argv, argc);

	else if (strcasecmp(cmd, "help") == 0)
		dos_help(argv, argc);

	else if (strcasecmp(cmd, "mkdir") == 0)
		dos_mkdir(argv, argc);
	else if (strcasecmp(cmd, "md") == 0)
		dos_mkdir(argv, argc);

	else if (strcasecmp(cmd, "pause") == 0)
		dos_pause();

	else if (strcasecmp(cmd, "rem") == 0) {}

	else if (strcasecmp(cmd, "rmdir") == 0)
		dos_rmdir(argv, argc);
	else if (strcasecmp(cmd, "rd") == 0)
		dos_rmdir(argv, argc);

	else if (strcasecmp(cmd, "ren") == 0)
		dos_ren(argv, argc);

	else if (strcasecmp(cmd, "touch") == 0)
		dos_touch(argv, argc);

	else if (strcasecmp(cmd, "type") == 0)
		dos_type(argv, argc);

	else if (strcasecmp(cmd, "ver") == 0)
		dos_ver();

	else
		exec_noext(cmd, ext, ext_cnt);

	if (echo && !isbatfile)
		putchar('\n');
}
