#include "dos_cmds.h"
#include "dos_const.h"
#include "print.h"
#include "conio.h"
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
#include <errno.h>
#include <sys/statvfs.h>

bool echo = true;

void 
dos_box (char **argv, int argc)
{
	int i, msgsz, msgidx, j;

	msgsz = msgidx = 0;

	for (i = 0; i < argc; i++) {
		/* +1 for space */
		msgsz += strlen(argv[i]) + 1;
	}

	char msg[msgsz + 1];

	for (i = 0; i < argc; i++) {
		for (j = 0; argv[i][j] != 0; j++)
			msg[msgidx++] = argv[i][j];
		if (i < argc - 1)
			msg[msgidx++] = ' ';
	}
	msg[msgidx] = 0;

	print_box(msg);
}

void
dos_copy (char **argv, int argc)
{
	char *file, *dest, buffer[256];
	int fd, dfd, mode, bytes_read;
	struct stat statbuf;

	if (argc != 2) {
		puts("The syntax of the command is incorrect.\n");
		return;
	}

	undosify_dir(argv[0]);
	undosify_dir(argv[1]);

	file = argv[0];
	dest = argv[1];
	fd = open(file, O_RDONLY);

	if (fd < 0) {
		perror("open");
		return;
	}
	
	lstat(file, &statbuf);
	mode = statbuf.st_mode;

	if (access(dest, F_OK) == 0) {
		puts("File already exists.");
		return;
	}
	if (access(file, F_OK) != 0) {
		puts("File does not exist.");
		return;
	}

	creat(dest, mode);
	dfd = open(dest, O_RDWR);

	if (dfd < 0) {
		perror("open");
		return;
	}

	memset(buffer, 0, sizeof(buffer));
	bytes_read = 0;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
		if (write(dfd, buffer, bytes_read) != bytes_read) {
			perror("write");
			return;
		}
		memset(buffer, 0, sizeof(buffer));
	}

	close(fd);
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

	closedir(dir);

	return maxlen;
}

int
print_ent_info (char *file, char *d_name, 
                 int *dirs, int *files, size_t *bytes,
		 int maxlen, int *linesz, bool b, bool w)
{
	time_t mod_time;
	struct tm *timeinfo;
	struct stat statbuf;
	struct winsize ws;
	char timebuf[81];

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

	if (lstat(file, &statbuf) == -1) {
		puts(file);
		perror("lstat");
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
		*linesz += maxlen;
		if (*linesz > ws.ws_col) {
			*linesz = 0;
			putchar('\n');
			fflush(stdout);
		}
		if (S_ISDIR(statbuf.st_mode))
			printf("[%s]%-*c", d_name, maxlen, ' ');
		else
			printf("%-*s", maxlen + 2, d_name);
	}
	else {
		printf("%-*s", maxlen, d_name);
		printf("    %s    ",
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
	int i, maxlen, dirs, files, linesz, nsa, farg, filesz;
	size_t bytes;
	char path[PATH_MAX + 1], timebuf[81],
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
			undosify_dir(argv[i]);

			filesz += strlen(argv[i]);
			filesz *= sizeof(char);
			file = realloc(file, filesz);

			if (!file) {
				perror("realloc");
				return;
			}

			memset(file, 0, filesz);
			strcpy(file, path);
			strcat(file, "/");
			strcat(file, argv[i]);
			farg = i;
			nsa++;
		}
	}
	
	maxlen = get_longest_name(file);

	if (!b) {
		char dpath[filesz];
		strcpy(dpath, file);
		dosify_dir(dpath);
		printf("Directory of %s.\n", dpath);
	}

	if (nsa != 0) {
		if (lstat(file, &statbuf) == -1) {
			perror("lstat");
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
						maxlen, &linesz, b, w) != 0) {
						return;
					}
				}
			}
			else {
				perror("opendir");
				free(file);
				return;
			}

			closedir(dir);
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
			               maxlen, &linesz, b, w) != 0) {
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
		printf("\n% 8d File(s)  % 21ld Byte(s).\n", files, bytes);
		printf("% 8d Dir(s).\n", dirs);
	}
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

	undosify_dir(argv[argc - 1]);
	chdir(argv[argc - 1]);
}

void
dos_cls ()
{
	clrscr();
}

void
dos_del (char **argv, int argc)
{
	int i;
	struct stat statbuf;

	if (argc < 1) {
		puts("Illegal Path.");
		return;
	}
	for (i = 0; i < argc; i++) {
		undosify_dir(argv[i]);
		
		if (lstat(argv[i], &statbuf) == -1) {
			if (errno == ENOENT)
				puts("Illegal Path.");
			else {
				perror("lstat");
			}
			return;
		}
		if (S_ISDIR(statbuf.st_mode)) {
			return;
		}
		if (remove(argv[i]) == -1) {
			perror("remove");
			return;
		}
	}
}

void
dos_echo (char **argv, int argc)
{
	int i, arglen;

	if (argc == 1) {
		if (strcasecmp(argv[0], "on") == 0) {
			echo = true;
			return;
		}
		else if (strcasecmp(argv[0], "off") == 0) {
			echo = false;
			return;
		}
	}

	for (i = 0; i < argc; i++) {
		arglen = strlen(argv[i]);

		write(STDOUT_FILENO, argv[i], arglen);
		putchar(' ');
		fflush(stdout);
	}

	putchar('\n');
}

void
dos_exit ()
{
	exit(0);
}

void
dos_fc (char **argv, int argc)
{
	struct stat statbuf;
	size_t f1sz, f2sz;
	int f1fd, f2fd;
	char f1ch, f2ch;
	bool neq;

	neq = false;

	if (argc != 2) {
		puts("The syntax of the command is incorrect.");
		return;
	}

	undosify_dir(argv[0]);
	undosify_dir(argv[1]);

	if (lstat(argv[0], &statbuf) != 0) {
		perror("stat");
		return;
	}

	f1sz = statbuf.st_size;

	if (lstat(argv[1], &statbuf) != 0) {
		perror("stat");
		return;
	}

	f2sz = statbuf.st_size;

	if (f1sz != f2sz)
		goto files_differ;

	f1fd = open(argv[0], O_RDONLY);
	f2fd = open(argv[1], O_RDONLY);

	if (f1fd < 0) {
		perror("open");
		return;
	}
	if (f2fd < 0) {
		perror("open");
		return;
	}

	while (read(f1fd, &f1ch, 1) == 1
	    && read(f2fd, &f2ch, 1) == 1) {
		if (f1ch != f2ch) {
			neq = true;
			break;
		}
	}

	close(f1fd);
	close(f2fd);

	if (neq == true)
		goto files_differ;
	else
		goto files_equal;

	files_differ:
		puts("Files are different.");
		return;
	files_equal:
		puts("Files are identical.");
		return;
}

void
dos_free (char **argv, int argc)
{
	struct statvfs stfs;
	size_t total_b, used_b, free_b;
	int i;
	bool h;

	if (statvfs("/", &stfs) != 0) {
		perror("statvfs");
		return;
	}

	free_b  = stfs.f_bavail * stfs.f_frsize;
	total_b = stfs.f_blocks * stfs.f_frsize;
	used_b  = total_b - free_b;

	for (i = 0; i < argc; i++) {
		if (strcasecmp(argv[i], "/h") == 0)
			h = true;
		else {
			puts("Illegal switch.");
			return;
		}
	}

	if (h) {
		putchar('\n');
		print("Total: ");
		print_readable_bytes(total_b);
		print("Used:  ");
		print_readable_bytes(used_b);
		print("Free:  ");
		print_readable_bytes(free_b);
		return;
	}

	printf("\n"
	       "Total: %20zu\n"
	       "Used:  %20zu\n"
	       "Free:  %20zu\n",
	       total_b,
	       used_b,
	       free_b
	);
}

void
dos_help (char **argv, int argc)
{
	int i;
	bool all;

	all = false;

	puts("<BOX     > Display messages in a box.\n"
	     "<CD      > Displays/changes the current directory.\n"
	     "<CHDIR   > Displays/changes the current directory.\n"
	     "<CLS     > Clear screen.\n"
	     "<COPY    > Copy file.\n"
	     "<DEL     > Removes one or more files.\n"
	     "<DELETE  > Removes one or more files.\n"
	     "<DIR     > Directory View.\n"
	     "<ERASE   > Removes one or more files.\n"
	     "<ECHO    > Display messages.\n"
	     "<EXIT    > Exit from the shell.\n"
	     "<FC      > Compare two files.\n"
	     "<FREE    > Display free disk space.\n"
	     "<HELP    > Show help.\n"
	     "<MKDIR   > Make Directory.\n"
	     "<MD      > Make Directory.\n"
	     "<PAUSE   > Wait for 1 keystroke to continue.\n"
	     "<RMDIR   > Remove Directory.\n"
	     "<RD      > Remove Directory.\n"
	     "<REM     > Add comments in a batch file.\n"
	     "<REN     > Rename file.\n"
	     "<TOUCH   > Make File.\n"
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
dos_mkdir (char **argv, int argc)
{
	if (argc != 1) {
		puts("The syntax of the command is incorrect.");
		return;
	}

	undosify_dir(argv[0]);

	if (access(argv[0], F_OK) == 0) {
		puts("Path already exists.");
		return;
	}

	if (mkdir(argv[0], 0755) == -1) {
		perror("mkdir");
		return;
	}
}

void
dos_pause ()
{
	puts("Press any key to continue.");
	(void)getch();
}

void
dos_rmdir (char **argv, int argc)
{
	int i;
	struct stat statbuf;

	if (argc < 1) {
		puts("Illegal Path.");
		return;
	}
	for (i = 0; i < argc; i++) {
		undosify_dir(argv[i]);

		if (lstat(argv[i], &statbuf) == -1) {
			if (errno == ENOENT)
				puts("Illegal Path.");
			else {
				perror("lstat");
			}
			return;
		}
		if (!S_ISDIR(statbuf.st_mode)) {
			return;
		}
		if (remove(argv[i]) == -1) {
			perror("remove");
			return;
		}
	}
}

void
dos_ren (char **argv, int argc)
{
	char src[PATH_MAX + 1], dir[PATH_MAX + 1],
	     dst[PATH_MAX + 4], *lastslash;

	if (argc != 2) {
		puts("The syntax of the command is incorrect.");
		return;
	}

	undosify_dir(argv[0]);
	undosify_dir(argv[1]);

	strncpy(src, argv[0], PATH_MAX + 1);
	lastslash = strrchr(src, '/');

	if (lastslash == NULL)
		strcpy(dir, ".");
	else {
		*lastslash = 0;
		strcpy(dir, src);
	}

	snprintf(dst, sizeof(dst), "%s/%s", dir, argv[1]);

	if (access(dst, F_OK) == 0) {
		puts("File already exists.");
		return;
	}
	if (access(argv[0], F_OK) != 0) {
		puts("File does not exist.");
		return;
	}

	if (rename(argv[0], dst) != 0)
		perror("rename");
}

void
dos_touch (char **argv, int argc)
{
	if (argc != 1) {
		puts("The syntax of the command is incorrect.");
		return;
	}

	undosify_dir(argv[0]);

	if (access(argv[0], F_OK) == 0) {
		puts("File already exists.");
		return;
	}
	
	creat(argv[0], 0755);
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
		undosify_dir(argv[i]);

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
dos_ver ()
{
	puts("DOS version " DOS_VERSION);
}
