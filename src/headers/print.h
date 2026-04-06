#ifndef DOS_PRINT_H
# define DOS_PRINT_H

#include <stddef.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define PBYTE_SIZE (1125899906842624ULL)
#define TBYTE_SIZE (   1099511627776ULL)
#define GBYTE_SIZE (      1073741824ULL)
#define MBYTE_SIZE (         1048576ULL)
#define KBYTE_SIZE (            1024ULL)

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
void dosify_dir
	(char *path);
void undosify_dir
	(char *path);
void print_path
	();
void print_readable_bytes
	(size_t bytes);
int readprompt
	(char **buffer, int *bytes, bool *buf_freeable);
int dos_read
	(char *buffer, size_t max);

static inline ssize_t
print (const char *msg)
{
	return write(STDOUT_FILENO, msg, strlen(msg));
}

static inline ssize_t
fprint (const char *msg, int fd)
{
	return write(fd, msg, strlen(msg));
}

#endif /* DOS_PRINT_H */
