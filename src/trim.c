#include "headers/trim.h"

#include <string.h>
#include <stdbool.h>
#include <ctype.h>

bool
in_set (char c, const char* cs)
{
	if (!cs) return isspace((unsigned char)c);
	for (; *cs; cs++) 
		if (c == *cs) return true;
	return false;
}

char *
trim (char *s, const char *cs)
{
	char *save;

	if (!s) return s;
	while (*s && in_set(*s, cs)) s++;
	if (!*s) return s;

	save = s + strlen(s) - 1;

	while (save >= s && in_set(*save, cs)) *save-- = 0;
	return s;
}

char *
triml (char *s, const char *cs)
{
	if (!s) return s;
	while (*s && in_set(*s, cs)) s++;
	return s;
}

char *
trimr (char* s, const char* cs)
{
	char *save;

	if (!s) return s;

	save = s + strlen(s) - 1;

	while (save >= s && in_set(*save, cs)) *save-- = 0;
	return s;
}
