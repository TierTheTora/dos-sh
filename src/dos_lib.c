#include "dos_lib.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

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
