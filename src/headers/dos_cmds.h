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

extern bool echo;

void dos_box
	(char **argv, int argc);
void dos_call
	(char **argv, int argc);
void dos_color
	(char **argv, int argc);
void dos_cd
	(char **argv, int argc);
void dos_cls
	();
void dos_copy
	(char **argv, int argc);
void dos_del
	(char **argv, int argc);
void dos_dir
	(char **argv, int argc);
void dos_echo
	(char **argv, int argc);
void dos_exit
	();
void dos_fc
	(char **argv, int argc);
void dos_free
	(char **argv, int argc);
void dos_help
	(char **argv, int argc);
void dos_mkdir
	(char **argv, int argc);
void dos_pause
	();
void dos_rmdir
	(char **argv, int argc);
void dos_ren
	(char **argv, int argc);
void dos_touch
	(char **argv, int argc);
void dos_type
	(char **argv, int argc);
void dos_ver
	();

#endif /* DOS_CMDS_H */
