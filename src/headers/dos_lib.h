#ifndef DOS_LIB_H
# define DOS_LIB_H

#include <stdint.h>
#include <sys/types.h>
#include "dos_const.h"

#define SEG_OFF(seg, off) ((WORD)((seg) << 4) + (off))
#define BETWEEN(memptr) (memptr % memsz)
#define BETWEENEQ(memptr) (memptr %= memsz)

#define SAFE_IP_PLUS(num) (BETWEEN((ipidx + num)))
#define SAFE_PLUS(memptr, num) (BETWEEN((memptr + num)))

#define DOS_SYSCALL(num)
#define INT21

#define CF 0x01
#define PF 0x04
#define OF 0x08
#define AF 0x10
#define ZF 0x40
#define SF 0x80

enum BITS {
	_8BIT  = 0,
	_16BIT = 1,
};

typedef uint8_t		BYTE;
typedef uint16_t	WORD;
typedef uint32_t	DWORD;
typedef uint64_t	QWORD;
typedef int32_t		HANDLE;
typedef _Bool		BOOL;
typedef int64_t		memptr_t;

typedef struct {
	union { struct { BYTE AL, AH; }; WORD AX; };
	union { struct { BYTE BL, BH; }; WORD BX; };
	union { struct { BYTE CL, CH; }; WORD CX; };
	union { struct { BYTE DL, DH; }; WORD DX; };
	WORD SI, DI, BP, SP;
	WORD CS, DS, ES, FS, GS, SS;
	WORD IP;
	BYTE flags;
} REGS;

#define LABELN_MAX 30

typedef struct {
	char name[31];
	size_t line;
} label;

extern BYTE *MEMORY;
extern HANDLE *handles;
extern BOOL memory_freeable, handles_freeable;
extern label *labels;
extern size_t labels_n, lbl_cnt;
extern DWORD ticks;

int init_dos
	();
void init_handles
	();
HANDLE new_handle
	();
void int21h
	(REGS *r);
void runcom
	(REGS *r, int fd, size_t sz);
void runbat
	(int fd);

#endif /* DOS_LIB_H */
