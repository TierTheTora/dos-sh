#ifndef DOS_CONST_H
# define DOS_CONST_H

extern int ERRORLEVEL;

#define ERRLVL_NONE     0
#define ERRLVL_MILD     1
#define ERRLVL_MEDIUM   2
#define ERRLVL_SEVERE   3
#define ERRLVL_CRITICAL 4

#define DOS_VERSION "1.0.0"

#define MEM_MAX       0xFFFFF
#define HANDLES_MAX   0xFFFF
#define DOS_USER_HNDL 5
#define HANDLE_UNUSED (-1)

#define DOS_STDIN  0
#define DOS_STDOUT 1
#define DOS_STDERR 2
#define DOS_STDAUX 3
#define DOS_STDPRN 4

#endif /* DOS_CONST_H */
