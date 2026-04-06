#include "headers/conio.h"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

int
kbhit ()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0L;
	tv.tv_usec = 0L;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	return select(1, &fds, NULL, NULL, &tv);
}

char *
cgets (const char *msg)
{
	return fgets((char *)msg, sizeof(msg), stdout);
}

int
putch (int c)
{
	return putchar(c);
}

int
cputs (const char *msg)
{
	return puts(msg);
}

int
getch ()
{
	struct termios old_sett, new_sett;
	int ch;

	tcgetattr(STDIN_FILENO, &old_sett);
	new_sett = old_sett;
	new_sett.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_sett);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &old_sett);

	return ch;
}

int
getche ()
{
	int ch;

	ch = getch();

	putchar(ch);

	return ch;
}

void
clrscr ()
{
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
}
