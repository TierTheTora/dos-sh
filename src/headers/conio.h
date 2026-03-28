#ifndef CONIO_H
# define CONIO_H

int kbhit
	();
char *cgets
	(const char *msg);
int putch
	(int c);
int cputs
	(const char *msg);
int getch
	();
int getche
	();

void clrscr
	();

#endif /* CONIO_H */
