#ifndef DOS_CMDS_H
# define DOS_CMDS_H

#include <stdbool.h>
#include <sys/types.h>

enum {
	DOSCOLOR_BLACK	= 0,
	DOSCOLOR_BLUE	= 1,
	DOSCOLOR_GREEN	= 2,
	DOSCOLOR_AQUA	= 3,
	DOSCOLOR_RED	= 4,
	DOSCOLOR_PURPLE	= 5,
	DOSCOLOR_YELLOW	= 6,
	DOSCOLOR_WHITE	= 7,
	DOSCOLOR_GRAY	= 8,

	DOSCOLOR_LIGHT_BLUE	= 9,
	DOSCOLOR_LIGHT_GREEN	= 10,
	DOSCOLOR_LIGHT_AQUA	= 11,
	DOSCOLOR_LIGHT_RED	= 12,
	DOSCOLOR_LIGHT_PURPLE	= 13,
	DOSCOLOR_LIGHT_YELLOW	= 14,
	DOSCOLOR_BRIGHT_WHITE	= 15,
};

enum {
	COLOR_BLACK	= 0,
	COLOR_RED	= 1,
	COLOR_GREEN	= 2,
	COLOR_YELLOW	= 3,
	COLOR_BLUE	= 4,
	COLOR_PURPLE	= 5,
	COLOR_AQUA	= 6,
	COLOR_WHITE	= 7,
	COLOR_GRAY	= 8,

	COLOR_LIGHT_RED		= 9,
	COLOR_LIGHT_GREEN	= 10,
	COLOR_LIGHT_YELLOW	= 11,
	COLOR_LIGHT_BLUE	= 12,
	COLOR_LIGHT_PURPLE	= 13,
	COLOR_LIGHT_AQUA	= 14,
	COLOR_BRIGHT_WHITE	= 15,
};

struct vartable {
	char name[256];
	char *value;
};

extern bool echo;
extern struct vartable *vars;
extern size_t vars_cnt, vars_max;

int init_vars
	(void);
int dos_box
	(char **argv, int argc);
int dos_call
	(char **argv, int argc);
int dos_choice
	(char **argv, int argc);
int dos_color
	(char **argv, int argc);
int dos_cd
	(char **argv, int argc);
int dos_cls
	(void);
int dos_copy
	(char **argv, int argc);
int dos_del
	(char **argv, int argc);
int dos_dir
	(char **argv, int argc);
int dos_echo
	(char **argv, int argc);
int dos_exit
	(void);
int dos_fc
	(char **argv, int argc);
int dos_free
	(char **argv, int argc);
int dos_help
	(char **argv, int argc);
int dos_mkdir
	(char **argv, int argc);
int dos_pause
	(void);
int dos_rmdir
	(char **argv, int argc);
int dos_ren
	(char **argv, int argc);
int dos_set
	(char **argv, int argc);
int dos_touch
	(char **argv, int argc);
int dos_type
	(char **argv, int argc);
int dos_ver
	(void);

#endif /* DOS_CMDS_H */
