#ifndef DOS_LIB_H
# define DOS_LIB_H

#include <stdint.h>
#include "dos_const.h"

#define SEG_OFF(seg, off) (((seg) << 4) + (off))
#define SET_CF(r) ((r)->CF |= 1)
#define CLEAR_CF(r) ((r)->CF &= ~1)

#define DOS_SYSCALL(num)
#define INT21

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t  HANDLE;
typedef _Bool    BOOL;

typedef struct {
	struct { union { BYTE AH, AL; }; WORD AX; };
	struct { union { BYTE BH, BL; }; WORD BX; };
	struct { union { BYTE CH, CL; }; WORD CX; };
	struct { union { BYTE DH, DL; }; WORD DX; };
	WORD SI, DI, BP, SP;
	WORD CS, DS, ES, FS, GS, SS;
	WORD IP;
	BOOL CF;
	BYTE flags;
} REGS;

extern BYTE MEMORY[MEM_MAX + 1];

void init_handles
	();
HANDLE new_handle
	();
void int86x
	(REGS *r);
void runcom
	(REGS *r, int fd);

#endif /* DOS_LIB_H */
