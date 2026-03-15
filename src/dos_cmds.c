#include "dos_cmds.h"
#include "dos_const.h"
#include "print.h"
#include "dos_lib.h"
#include <linux/limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>

void
dos_exit ()
{
	exit(0);
}

int
get_longest_name (char *path)
{
	DIR *dir;
	struct dirent *ent;
	int len, maxlen;

	dir = opendir(path);
	len = maxlen = 0;

	if (dir == NULL) {
		perror("opendir");
		return -1;
	}

	while ((ent = readdir(dir)) != NULL) {
		len = strlen(ent->d_name);
		if (len > maxlen)
			maxlen = len;
	}

	/* add 2 for additional padding */
	return maxlen + 2;
}

int
print_ent_info (char *file, char *d_name, 
                 int *dirs, int *files, int *bytes,
		 int maxlen, bool b, bool w)
{
	time_t mod_time;
	struct tm *timeinfo;
	struct stat statbuf;
	struct winsize ws;
	char timebuf[81];
	int linesz;

	linesz = 0;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

	if (stat(file, &statbuf) == -1) {
		perror("stat");
		return -1;
	}

	mod_time = statbuf.st_mtime;
	timeinfo = localtime(&mod_time);

	strftime(timebuf, sizeof(timebuf),
	         "%d-%m-%Y %H:%M", timeinfo);

	if (S_ISDIR(statbuf.st_mode))
		(*dirs)++;
	else
		(*files)++;
	*bytes += statbuf.st_size;

	if (b)
		puts(d_name);
	else if (w) {
		linesz += maxlen;
		if (linesz > ws.ws_col) {
			linesz = 0;
			putchar('\n');
			fflush(stdout);
		}
		printf("%-*s", maxlen, d_name);
	}
	else {
		printf("%-*s", maxlen, d_name);
		printf("    %s     ",
		       S_ISDIR(statbuf.st_mode) ?
		         "<DIR>" :
			 "     "
		);
		printf("% 8ld %s\n", statbuf.st_size,
		       timebuf);
	}

	return 0;
}

void
dos_dir (char **argv, int argc)
{
	/* nsa = non-switch args */
	int i, maxlen, dirs, files, bytes, linesz, nsa, farg, filesz;
	char path[PATH_MAX + 1], dpath[PATH_MAX + 1], timebuf[81],
	     *file;
	DIR *dir;
	time_t mod_time;
	bool w, b, p, s;
	struct stat statbuf;
	struct tm *timeinfo;
	struct dirent *ent;

	dirs = files = bytes = linesz = nsa = filesz = 0;
	w = b = p = s = false;

	if (getcwd(path, sizeof(path)) == NULL) {
		perror("getcwd");
		return;
	}

	filesz = strlen(path) + 2;
	filesz *= sizeof(char);
	file = malloc(filesz);

	if (!file) {
		perror("malloc");
		return;
	}

	memset(file, 0, filesz);
	strcpy(file, path);
	strcat(file, "/");

	for (i = 0; i < argc; i++) {
		if (nsa > 1) {
			puts("Illegal Path.");
			return;
		}
		if (strcasecmp(argv[i], "/w") == 0)
			w = true;
		else if (strcasecmp(argv[i], "/b") == 0)
			b = true;
		else if (strcasecmp(argv[i], "/p") == 0)
			p = true;
		else if (strcasecmp(argv[i], "/s") == 0)
			s = true;
		else if (argv[i][0] == '/') {
			printf("Illegal switch: %s.\n", argv[i]);
			return;
		}
		else {
			strcat(file, argv[i]);
			farg = i;
			nsa++;
		}
	}

	if (!b) {
		strncpy(dpath, path, sizeof(path));
		dosify_dir(dpath);
		printf("Directory of %s.\n", dpath);
		maxlen = get_longest_name(path);
	}

	if (nsa != 0) {
		if (stat(file, &statbuf) == -1) {
			perror("stat");
			return;
		}

		if (S_ISDIR(statbuf.st_mode)) {
			filesz = strlen(path) + strlen(argv[farg]) + 2;
			filesz *= sizeof(char);
			file = realloc(file, filesz);

			if (!file) {
				perror("realloc");
				return;
			}

			memset(file, 0, filesz);
			strcpy(file, path);
			strcat(file, "/");
			strcat(file, argv[farg]);

			if ((dir = opendir(file)) != NULL) {
				while ((ent = readdir(dir)) != NULL) {
					filesz = strlen(path)
					       + strlen(argv[farg])
					       + 3
					       + strlen(ent->d_name);
					filesz *= sizeof(char);
					file = realloc(file, filesz);

					if (!file) {
						perror("realloc");
						return;
					}

					memset(file, 0, filesz);
					strcpy(file, path);
					strcat(file, "/");
					strcat(file, argv[farg]);
					strcat(file, "/");
					strcat(file, ent->d_name);

					if (print_ent_info(file, ent->d_name,
						&dirs, &files, &bytes,
						maxlen, b, w) != 0) {
						return;
					}
				}
			}
			else {
				perror("opendir");
				free(file);
				return;
			}
			free(file);
			if (!b)
				goto print_info;
			return;
		}

		puts(argv[farg]);

		if (!b) {
			mod_time = statbuf.st_mtime;
			timeinfo = localtime(&mod_time);

			strftime(timebuf, sizeof(timebuf),
				 "%d-%m-%Y %H:%M", timeinfo);

			if (S_ISDIR(statbuf.st_mode))
				dirs++;
			else
				files++;
			bytes += statbuf.st_size;

			goto print_info;
		}
	}
	if ((dir = opendir(path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			filesz = strlen(path) + 2
			       + strlen(ent->d_name);
			filesz *= sizeof(char);
			file = realloc(file, filesz);

			if (!file) {
				perror("realloc");
				return;
			}

			memset(file, 0, filesz);
			strcpy(file, path);
			strcat(file, "/");
			strcat(file, ent->d_name);

			if (print_ent_info(file, ent->d_name,
			               &dirs, &files, &bytes,
			               maxlen, b, w) != 0) {
				return;
			}
		}
		closedir(dir);
	}
	else {
		perror("opendir");
		free(file);
		return;
	}
	free(file);
	if (!b) {
		print_info:
		printf("\n% 8d File(s)  % 21d Byte(s).\n", files, bytes);
		printf("% 8d Dir(s).\n", dirs);
	}
}

void
dos_help (char **argv, int argc)
{
	int i;
	bool all;

	all = false;

	puts("<DIR     > Directory View.\n"
	     "<CHDIR   > Displays/changes the current directory.\n"
	     "<CD      > Displays/changes the current directory.\n"
	     "<CLS     > Clear screen.\n"
	     "<ECHO    > Display messages.\n"
	     "<EXIT    > Exit from the shell.\n"
	     "<HELP    > Show help.\n"
	     "<PAUSE   > Wait for 1 keystroke to continue.\n"
	     "<TYPE    > Display the contents of a text-file.\n"
	     "<VER     > View the DOS version."
	);

	for (i = 0; i < argc; i++) {
		if (strcasecmp(argv[i], "/all") == 0) {
			all = true;
		}
	}
	if (all == true)
		puts("More commands coming soon . . ."
		);
}

void
dos_cd (char **argv, int argc)
{
	char cwd[PATH_MAX];

	if (argc < 1) {
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			dosify_dir(cwd);
			puts(cwd);
		}
		return;
	}

	chdir(argv[argc - 1]);
}

void
dos_ver ()
{
	puts("DOS version " DOS_VERSION);
}

void
dos_echo (char **argv, int argc)
{
	int i, arglen;

	for (i = 0; i < argc; i++) {
		arglen = strlen(argv[i]);

		write(STDOUT_FILENO, argv[i], arglen);
		putchar(' ');
		fflush(stdout);
	}

	putchar('\n');
}

void
dos_pause ()
{
	puts("Press any key to continue.");
	(void)getch();
}

void
dos_type (char **argv, int argc)
{
	int i, fd;
	char c;

	if (argc == 0) {
		puts("The syntax of the command is incorrect.");
		return;
	}
	for (i = 0; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		
		if (fd == -1) {
			printf("The file %s does not exist.",
			       argv[i]);
			continue;
		}

		while (read(fd, &c, 1) == 1) {
			putchar(c);
		}
		
		close(fd);
	}
}

void
dos_cls ()
{
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
}
