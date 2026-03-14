#ifndef DOS_PRINT_H
# define DOS_PRINT_H

#include <sys/types.h>
#include <stdarg.h>

struct substr_info {
	size_t longest_substr;
	size_t delim_cnt;
};

void put_tabl_h
	(size_t len);
struct substr_info get_longest_substr
	(const char *msg, char delim);
void print_box
	(const char *msg);

#endif /* DOS_PRINT_H */
