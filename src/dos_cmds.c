#include "headers/dos_cmds.h"
#include "headers/dos_exec.h"
#include "headers/dos_const.h"
#include "headers/print.h"
#include "headers/conio.h"

#include <asm-generic/errno-base.h>
#include <ctype.h>
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
size_t vars_cnt, vars_max;
struct vartable *vars;

int
init_vars ()
{
	vars_cnt = 0;
	vars_max = 256;
	vars = calloc(vars_max, sizeof *vars);

	if (vars == NULL) {
		perror("calloc");
		return -1;
	}

	return 0;
}

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
dos_call (char **argv, int argc)
{
	const char *ext[] = { ".bat", "" };
	int ext_cnt = 2;

	if (argc < 1) {
		puts(DOSSTR_ILLEGAL_SYN);
		return;
	}

	exec_noext(argv[0], ext, ext_cnt, X_EXEC_VERBOSE);
}

int
nibble_to_color (char nibble)
{
	int nibnum = -1;
	nibble = tolower((int)nibble);

	if (nibble >= '0' && nibble <= '9')
		nibnum = nibble - '0';
	if (nibble >= 'a' && nibble <= 'f')
		nibnum = nibble - 'a' + 10;
	if (nibnum == -1)
		return -1;

	if (nibnum == DOSCOLOR_BLUE)
		nibnum = COLOR_BLUE;
	else if (nibnum == DOSCOLOR_RED)
		nibnum = COLOR_RED;
	if (nibnum == DOSCOLOR_AQUA)
		nibnum = COLOR_AQUA;
	else if (nibnum == DOSCOLOR_YELLOW)
		nibnum = COLOR_YELLOW;
	if (nibnum == DOSCOLOR_LIGHT_BLUE)
		nibnum = COLOR_LIGHT_BLUE;
	else if (nibnum == DOSCOLOR_LIGHT_RED)
		nibnum = COLOR_LIGHT_RED;
	if (nibnum == DOSCOLOR_LIGHT_AQUA)
		nibnum = COLOR_LIGHT_AQUA;
	else if (nibnum == DOSCOLOR_LIGHT_YELLOW)
		nibnum = COLOR_LIGHT_YELLOW;
	return nibnum;
}

void
dos_color (char **argv, int argc)
{
	/* cant change the whole color of the terminal in linux */
	int n1, n2;

	if (argc != 1) {
		wrong_syntax:
		puts(DOSSTR_ILLEGAL_SYN);
		return;
	}
	if (strlen(argv[0]) != 2)
		goto wrong_syntax;

	n1 = nibble_to_color((int)argv[0][0]);
	n2 = nibble_to_color((int)argv[0][1]);

	if (n1 == -1 || n2 == -1)
		goto wrong_syntax;

	printf("\033[38;5;%dm\033[48;5;%dm", n1, n2);
	fflush(stdout);
}

void
dos_copy (char **argv, int argc)
{
	char *file, *dest, buffer[256];
	int fd, dfd, mode, bytes_read;
	struct stat statbuf;

	if (argc != 2) {
		puts(DOSSTR_ILLEGAL_SYN);
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
	char timebuf[81], fullpath[PATH_MAX + 2];
	int padding;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	snprintf(fullpath, sizeof(fullpath), "%s/%s", file, d_name);

	if (lstat(fullpath, &statbuf) == -1) {
		if (errno == EACCES) return 0;
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

	dosify_dir(d_name);

	if (b)
		puts(d_name);
	else if (w) {
		padding = maxlen + 4;

		if (*linesz > 0 && *linesz + padding > ws.ws_col) {
			*linesz = 0;
			putchar('\n');
		}
		if (S_ISDIR(statbuf.st_mode))
			printf("[%s]%-*c", d_name,
			       (int)(padding - strlen(d_name) - 2),
			       ' ');
		else
			printf("%-*s", padding, d_name);

		*linesz += padding;
	}
	else {
		printf("%-*s", maxlen, d_name);
		printf("    %s    ",
			S_ISDIR(statbuf.st_mode) ?
			"<DIR>" :
			"     "
		);
		printf("% 20ld %s\n", statbuf.st_size,
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
	     file[PATH_MAX + 1], real[PATH_MAX + 1];
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

	strcpy(file, path);

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
			strncpy(file, argv[i], PATH_MAX + 1);

			farg = i;
			nsa++;
		}
	}
	
	
	if (lstat(file, &statbuf) == -1) {
		perror("lstat");
		return;
	}

	if (S_ISDIR(statbuf.st_mode))
		maxlen = get_longest_name(file);
	else
		maxlen = strlen(file);

	if (!b) {
		if (realpath(file, real) == NULL) {
			perror("realpath");
			return;
		}

		dosify_dir(real);
		printf("Directory of %s.\n", real);
	}
	if (nsa != 0) {
		if (S_ISDIR(statbuf.st_mode)) {
			if ((dir = opendir(file)) != NULL) {
				while ((ent = readdir(dir)) != NULL) {
					if (print_ent_info(file, ent->d_name,
						&dirs, &files, &bytes,
						maxlen, &linesz, b, w) != 0) {
						return;
					}
				}
			}
			else {
				perror("opendir");
				return;
			}

			closedir(dir);

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
		return;
	}
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
	else if (argc == 0) {
		printf("ECHO is %s.", echo ? "on" : "off");
	}

	for (i = 0; i < argc; i++) {
		arglen = strlen(argv[i]);

		write(STDOUT_FILENO, argv[i], arglen);

		if (arglen > 0) putchar(' ');

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
		puts(DOSSTR_ILLEGAL_SYN);
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
	     "<CALL    > Start a batch file within another batch file.\n"
	     "<CD      > Displays/changes the current directory.\n"
	     "<CHDIR   > Displays/changes the current directory.\n"
	     "<CLS     > Clear screen.\n"
	     "<COLOR   > Set the terminal color.\n"
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
		puts(DOSSTR_ILLEGAL_SYN);
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
	puts(DOSSTR_PKEY);
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
		puts(DOSSTR_ILLEGAL_SYN);
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

bool
var_exists (char *varname)
{
	size_t i;

	for (i = 0; i < vars_cnt; i++)
		if (strcasecmp(vars[i].name, varname) == 0)
			return true;
	return false;
}

size_t
get_varidx (char *varname)
{
	size_t i;

	for (i = 0; i < vars_cnt; i++)
		if (strcasecmp(vars[i].name, varname) == 0)
			return i;
	return -1;
}

void
dos_set (char **argv, int argc)
{
	char varname[256], *varptr;
	int varchars, valuelen, varidx;
	size_t i;
	struct vartable *tmp;
	varchars = valuelen = 0;
	varidx = vars_cnt;

	if (argc < 1) {
		for (i = 0; i < vars_cnt; i++) {
			printf("%s=%s\n", vars[i].name, vars[i].value);
		}
		return;
	}
	while (**argv != '=' && **argv != 0) {
		varname[varchars++] = **argv;
		(*argv)++;
	}
	if (**argv != '=' || varchars == 0) {
		puts(DOSSTR_ILLEGAL_SYN);
		return;
	}
	if (varchars > 255) {
		puts("SET: variable name exceeds 255 character limit.");

		return;
	}
	
	varname[varchars] = 0;
	/* skip '=' */
	(*argv)++;

	if (var_exists(varname)) {
		varidx = get_varidx(varname);

		free(vars[varidx].value);
	}
	else {
		if (vars_cnt >= vars_max) {
			vars_max *= 2;
			tmp = realloc(vars, vars_max * sizeof(*vars));

			if (tmp == NULL) {
				perror("realloc");

				return;
			}

			vars = tmp;
		}

		varidx = vars_cnt++;
	}
	for (i = 0; argv[i] != NULL; i++) {
		valuelen += strlen(argv[i]);
		if (argv[i + 1] != NULL) valuelen++;
	}

	valuelen++;
	vars[varidx].value = calloc(valuelen, sizeof(char));

	if (vars[varidx].value == NULL) {
		perror("calloc");

		return;
	}

	varptr = vars[varidx].value;

	for (i = 0; argv[i] != NULL; i++) {
		strcpy(varptr, argv[i]);

		varptr += strlen(argv[i]);

		if (argv[i + 1] != NULL)
			*varptr++ = ' ';
	}

	strcpy(vars[varidx].name, varname);
}

void
dos_touch (char **argv, int argc)
{
	if (argc != 1) {
		puts(DOSSTR_ILLEGAL_SYN);
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
		puts(DOSSTR_ILLEGAL_SYN);
		return;
	}
	for (i = 0; i < argc; i++) {
		undosify_dir(argv[i]);

		fd = open(argv[i], O_RDONLY);
		
		if (fd == -1) {
			puts("File does not exist.");

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
