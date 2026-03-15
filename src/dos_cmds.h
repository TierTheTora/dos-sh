#ifndef DOS_CMDS_H
# define DOS_CMDS_H

void dos_exit
	();
void dos_help
	(char **argv, int argc);
void dos_ver
	();
void dos_echo
	(char **argv, int argc);
void dos_pause
	();
void dos_type
	(char **argv, int argc);
void dos_cls
	();
void dos_cd
	(char **argv, int argc);
void dos_dir
	(char **argv, int argc);

#endif /* DOS_CMDS_H */
