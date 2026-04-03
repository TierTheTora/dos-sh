#ifndef MAIN_H
# define MAIN_H

extern char *buffer;
extern char *tmpbuf;
extern bool buf_freeable;
extern struct opt args;
extern struct termios oldt;

int init_term
	();
void restore_term
	();
void kill_dos
	();

#endif /* MAIN_H */
