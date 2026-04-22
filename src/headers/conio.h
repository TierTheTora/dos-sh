#ifndef CONIO_H
# define CONIO_H

int kbhit
	(void);
char *cgets
	(const char *msg);
int putch
	(int c);
int cputs
	(const char *msg);
int getch
	(void);
int getche
	(void);
void clrscr
	(void);

#endif /* CONIO_H */
