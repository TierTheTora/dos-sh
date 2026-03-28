#include "headers/dos_lib.h"
#include "headers/dos_const.h"
#include "headers/conio.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

HANDLE handles[HANDLES_MAX + 1] = { HANDLE_UNUSED };
int ERRORLEVEL = 0;
BYTE MEMORY[MEM_MAX + 1] = { 0 };

void
init_handles ()
{
	for (HANDLE i = 0; i < HANDLES_MAX; i++)
		handles[i] = HANDLE_UNUSED;

	handles[DOS_STDIN]  = STDIN_FILENO;
	handles[DOS_STDOUT] = STDOUT_FILENO;
	handles[DOS_STDERR] = STDERR_FILENO;
	
	/* there is no UNIX/LINUX STDAUX and STDPRN fd, so just use stderr.*/
	
	handles[DOS_STDAUX] = STDERR_FILENO;
	handles[DOS_STDPRN] = STDERR_FILENO;
}

HANDLE
new_handle ()
{
	int i;

	for (i = DOS_USER_HNDL; i < HANDLES_MAX; i++)
		if (handles[i] == HANDLE_UNUSED)
			return i;
	return -1;
}

void DOS_SYSCALL(0x01)
dos_sys_getche INT21 (REGS *r)
{
	int c;

	c = getche();
	r->AL = (BYTE)c;
	putchar(c);
} 

void DOS_SYSCALL(0x02)
dos_sys_putchar INT21 (REGS *r)
{
	putchar(r->DL);
	fflush(stdout);
}

void DOS_SYSCALL(0x09)
dos_sys_print INT21 (REGS *r)
{
	DWORD addr = SEG_OFF(r->DS, r->DX);
	
	while (MEMORY[addr] != '$') {
		putchar(MEMORY[addr]);
		addr++;
	}
}

HANDLE DOS_SYSCALL(0x3D)
dos_sys_open INT21 (REGS *r)
{
	DWORD addr = SEG_OFF(r->DS, r->DX);
	BYTE *name = &MEMORY[addr];
	int flags = O_RDONLY, fd;
	HANDLE h;

	switch (r->AL) {
	case 1:
		flags = O_WRONLY;
		break;
	case 2:
		flags = O_RDWR;
		break;
	}
	
	fd = open((char *)name, flags);
	h = new_handle();

	if (fd < 0) {
		r->AX = 0xFFFF;
		return 0xFFFF;
	}
	if (h == -1) {
		close(fd);
		r->AX = 0xFFFF;
		return 0xFFFF;
	}

	handles[h] = fd;
	r->AX = h;

	return h;
}

void DOS_SYSCALL(0x3E)
dos_sys_close INT21 (REGS *r)
{
	close(handles[r->BX]);
	handles[r->BX] = HANDLE_UNUSED;
}

void DOS_SYSCALL(0x3F)
dos_sys_read INT21 (REGS *r)
{
	DWORD addr = SEG_OFF(r->DS, r->DX);
	int fd = handles[r->BX];
	int n = read(fd, &MEMORY[addr], r->CX);

	if (n < 0) {
		r->AX = 0;
		SET_CF(r);
	}
	else {
		r->AX = n;
		CLEAR_CF(r);
	}
}

void DOS_SYSCALL(0x40)
dos_sys_write INT21 (REGS *r)
{
	DWORD addr = SEG_OFF(r->DS, r->DX);
	int fd = handles[r->BX];
	int n = write(fd, &MEMORY[addr], r->CX);

	if (n < 0) {
		r->AX = 0;
		SET_CF(r);
	}
	else {
		r->AX = n;
		CLEAR_CF(r);
	}
}

void DOS_SYSCALL(0x4C)
dos_sys_exit INT21 (REGS *r)
{
	ERRORLEVEL = r->AL;
	exit(ERRORLEVEL);
}

void
int86x (REGS *r)
{
	switch (r->AH) {
	case 0x01: dos_sys_getche (r); break;
	case 0x02: dos_sys_putchar(r); break;
	case 0x09: dos_sys_print  (r); break;
	case 0x3D: dos_sys_open   (r); break;
	case 0x3E: dos_sys_close  (r); break;
	case 0x3F: dos_sys_read   (r); break;
	case 0x40: dos_sys_write  (r); break;
	case 0x4C: dos_sys_exit   (r); break;
	default: printf("INT21h, AH=%02X not implemented.\n", r->AH);
	}
}

void
runcom (REGS *r, int fd)
{
	BYTE ch, ch2;
	DWORD filesz, ipidx;
	filesz = read(fd, &MEMORY[PRG_START], MEM_MAX - PRG_START);
	filesz += PRG_START;

	if (filesz <= 0) {
		perror("read");
		return;
	}

	r->CS = r->DS =
	r->ES = r->SS = 0;
	r->SP = MEM_MAX - 2;
	r->IP = PRG_START;
	ipidx = r->IP;

	while (ipidx < filesz) {
		ch = MEMORY[ipidx++];

		switch (ch) {
		/* mov $imm16, %dx */
		case 0xBA:
			if (ipidx + 1 >= filesz) {
				puts("Not enough arguments for 0xBA");
				return;
			}
			r->DX = MEMORY[ipidx] | (MEMORY[ipidx + 1] << 8);
			ipidx += 2;
			break;
		/* mov $imm8, %ah */
		case 0xB4:
			if (ipidx + 1 >= filesz) {
				puts("Not enough arguments for 0xB4");
				return;
			}
			r->AH = MEMORY[ipidx++];
			break;
		/* int $imm8 */
		case 0xCD:
			if (ipidx + 1 >= filesz) {
				puts("Not enough arguments for 0xCD");
				return;
			}
			ch2 = MEMORY[ipidx++];
			switch (ch2) {
			case 0x20:
				return;
			case 0x21:
				int86x(r);
				break;
			default:
				printf("INT%02Xh not implemented.\n", ch2);
			}
			break;
		/* ret */
		case 0xC3: {
			WORD newip;

			if (r->SP + 1 >= MEM_MAX) {
				puts("Stack underflow");
				return;
			}

			newip = MEMORY[r->SP] | (MEMORY[r->SP + 1] << 8);
			r->SP += 2;
			r->IP = newip;
			ipidx = r->IP;

			return;
		}
		default:
			printf("Unhandled opcode: %02X", ch);
		}
	}
}
