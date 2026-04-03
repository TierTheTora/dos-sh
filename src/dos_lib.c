#include "headers/dos_lib.h"
#include "headers/dos_const.h"
#include "headers/print.h"
#include "headers/conio.h"
#include "headers/parse_opt.h"
#include "headers/dos_exec.h"

#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

HANDLE *handles;
BYTE *MEMORY;
int ERRORLEVEL = 0;
BOOL memory_freeable = false;
BOOL handles_freeable = false;

DWORD
bitsnum (DWORD n)
{
	DWORD cnt = 0;

	while (n) {
		cnt += n & 1;
		n >>= 1;
	}
	return cnt;
}

BYTE *
get_r8 (REGS *r, BYTE i)
{
	switch (i) {
	case 0:  return &r->AL;
	case 1:  return &r->CL;
	case 2:  return &r->DL;
	case 3:  return &r->BL;
	case 4:  return &r->AH;
	case 5:  return &r->CH;
	case 6:  return &r->DH;
	case 7:  return &r->BH;
	default: return NULL;
	}
}

WORD *
get_r16 (REGS *r, BYTE i)
{
	switch (i) {
	case 0:  return &r->AX;
	case 1:  return &r->CX;
	case 2:  return &r->DX;
	case 3:  return &r->BX;
	case 4:  return &r->SP;
	case 5:  return &r->BP;
	case 6:  return &r->SI;
	case 7:  return &r->DI;
	default: return NULL;
	}
}

void
update_f8 (REGS *r, BYTE dst, BYTE src, WORD res)
{
	BYTE res8 = res & 0xFF;
	r->flags &= ~(CF | OF | PF | AF | ZF | SF);

	if (res > 0xFF)                        r->flags |= CF; 
	if (res8 == 0)                         r->flags |= ZF;
	if (res8 & 0x80)                       r->flags |= SF;
	if (bitsnum(res8) % 2 == 0)            r->flags |= PF;
	if (((dst & 0xF) + (src & 0xF)) > 0xF) r->flags |= AF;
	if (((dst & 0x80) == (src & 0x80))
	   &&(dst & 0x80) != (res8 & 0x80))    r->flags |= OF;
}

void
update_f16 (REGS *r, WORD dst, WORD src, DWORD res)
{
	WORD res16 = res & 0xFFFF;
	r->flags &= ~(CF | OF | PF | AF | ZF | SF);

	if (res > 0xFFFF)                          r->flags |= CF; 
	if (res16 == 0)                            r->flags |= ZF;
	if (res16 & 0x0080)                        r->flags |= SF;
	if (bitsnum(res16 & 0xFF) % 2 == 0)        r->flags |= PF;
	if (((dst & 0xF) + (src & 0xF)) > 0xF)     r->flags |= AF;
	if (((dst & 0x8000) == (src & 0x8000))
	   &&(dst & 0x8000) != (res16 & 0x8000))   r->flags |= OF;
}

int
init_dos ()
{
	MEMORY = calloc(MEM_MAX + 1, sizeof(BYTE));
	handles = malloc((HANDLES_MAX + 1) * sizeof(HANDLE));
	handles_freeable = memory_freeable = true;

	if (!MEMORY) {
		puts("Could not initalize memory");
		memory_freeable = false;
		return -1;
	}
	if (!handles) {
		puts("Could not initalize handles");
		handles_freeable = false;
		return -1;
	}

	memset(handles, HANDLE_UNUSED, HANDLES_MAX + 1);

	return 0;
}

WORD
calc_ea (REGS *r, BYTE modrm, WORD *ipidx)
{
	BYTE mod, rm;
	WORD disp, addr;
	mod  = (modrm >> 6) & 0x03;
	rm   = modrm & 0x07;
	disp = 0;

	switch (mod) {
	case 0:
		/* [rm] */
		disp = 0;
		break;
	case 1:
		/* [rm + disp8] */
		disp = MEMORY[(*ipidx)++];
		break;
	case 2:
		/* [rm + disp16 */
		disp = MEMORY[(*ipidx)] | (MEMORY[(*ipidx) + 1] << 8);
		(*ipidx) += 2;
		break;
	case 3:
		/* reg */
		return 0;
	}

	switch (rm) {
	case 0: addr = r->BX + r->SI; break;
	case 1: addr = r->BX + r->DI; break;
	case 2: addr = r->BP + r->SI; break;
	case 3: addr = r->BP + r->DI; break;
	case 4: addr = r->SI;         break;
	case 5: addr = r->DI;         break;
	case 6: addr = r->BP;         break;
	case 7: addr = r->BX;         break;
	default: return 0;
	}

	addr += disp;
	addr = SEG_OFF(r->DS, addr);

	return addr;
}

void
exec_add_r8 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm, *dst, *src, src_val, dst_val;
	WORD res, addr;

	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;
	src   = get_r8(r, reg);

	if (!src) {
		puts("Illegal source register");
		return;
	}

	src_val = *src;

	if (mod == 3) {
		dst = get_r8(r, rm);

		if (!dst) {
			puts("Illegal destination register");
			return;
		}

		dst_val = *dst;
		res = dst_val + src_val;
		*dst = res & 0xFF;

		update_f8(r, dst_val, src_val, res);
	}
	else {
		addr = calc_ea(r, modrm, ipidx);

		if (addr == 0 && modrm != 6) {
			puts("Failed to calculate effective address");
			return;
		}

		dst_val = MEMORY[addr];
		res = dst_val + src_val;
		MEMORY[addr] = res & 0xFF;

		update_f8(r, dst_val, src_val, res);
	}
}

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
	WORD addr = SEG_OFF(r->DS, r->DX);
	
	while (MEMORY[addr] != '$') {
		putchar(MEMORY[addr]);
		addr++;
	}
}

void DOS_SYSCALL(0x0A)
dos_sys_read INT21 (REGS *r)
{
	WORD addr;
	BYTE max_chars;
	char *buffer;
	int bytes_read;
	addr = SEG_OFF(r->DS, r->DX);
	max_chars = MEMORY[addr];

	fflush(stdout);

	buffer = (char *)(&MEMORY[addr + 2]);
	bytes_read = dos_read(buffer, max_chars);

	if (bytes_read < 0) {
		r->AL = 0;
		MEMORY[addr + 1] = 0;
		return;
	}

	MEMORY[addr + 1] = bytes_read;
	r->AL = bytes_read;
}

/*
HANDLE DOS_SYSCALL(0x3D)
dos_sys_open INT21 (REGS *r)
{
	WORD addr = SEG_OFF(r->DS, r->DX);
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
dos_sys_readf INT21 (REGS *r)
{
	DWORD addr = SEG_OFF(r->DS, r->DX);
	int fd = handles[r->BX];
	int n = read(fd, &MEMORY[addr], r->CX);

	r->AX = n;
	if (n < 0) (r);
	else       CLEAR_CF(r);
}
*/

void DOS_SYSCALL(0x40)
dos_sys_write INT21 (REGS *r)
{
	DWORD addr = SEG_OFF(r->DS, r->DX);
	int fd = handles[r->BX];
	int n = write(fd, &MEMORY[addr], r->CX);

	if (n < 0)
		r->AX = 0;
	else
		r->AX = n;
}

void
int86x (REGS *r)
{
	switch (r->AH) {
	case 0x01: dos_sys_getche (r); break;
	case 0x02: dos_sys_putchar(r); break;
	case 0x09: dos_sys_print  (r); break;
	case 0x0A: dos_sys_read   (r); break;
	/*
	case 0x3D: dos_sys_open   (r); break;
	case 0x3E: dos_sys_close  (r); break;
	case 0x3F: dos_sys_readf  (r); break;
	*/
	case 0x40: dos_sys_write  (r); break;
	default: printf("INT21h, AH=%02X not implemented.\n", r->AH);
	}
}

void
runcom (REGS *r, int fd)
{
	BYTE ch, ch2, modrm, reg, rm, *src, *dst;
	WORD *ipidx;
	int rret = read(fd, &MEMORY[PRG_START], MEM_MAX - PRG_START);

	if (rret == -1) {
		perror("read");
		return;
	}

	r->CS = r->DS =
	r->ES = r->SS = 0;
	r->SP = MEM_MAX - 2;
	r->IP = PRG_START;
	ipidx = &r->IP;

	while (true) {
		ch = MEMORY[(*ipidx)++];

		switch (ch) {
		/* add %r8, $imm/r8 */
		case 0x00:
			exec_add_r8(r, ipidx);
			break;
		/* mov %r8, %r8 */
		case 0x88:
			modrm = MEMORY[(*ipidx)++];
			reg   = (modrm >> 3) & 0x7;
			rm    = modrm & 0x7;
			src   = get_r8(r, reg);
			dst   = get_r8(r, rm);
			*dst = *src;
			break;
		/* mov $imm16, %dx */
		case 0xBA:
			r->DX = MEMORY[(*ipidx)] | (MEMORY[(*ipidx) + 1] << 8);
			(*ipidx) += 2;
			break;
		/* mov $imm8, %ah */
		case 0xB4:
			r->AH = MEMORY[(*ipidx)++];
			break;
		/* int $imm8 */
		case 0xCD:
			ch2 = MEMORY[(*ipidx)++];
			switch (ch2) {
			case 0x20:
				return;
			case 0x21:
				if (r->AH == 0x4C) return;
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
			(*ipidx) = r->IP;

			return;
		}
		/* jmp $imm8 */
		case 0xEB:
			   (*ipidx) = MEMORY[(*ipidx) + 1];
			   break;
		default:
			printf("Unhandled opcode: 0x%02X", ch);
			return;
		}
	}
}

void
runbat (int fd)
{
	char *line, *tmpbuf, *tok;
	int sz, ch, bytes;
	struct opt arg;
	sz = 256;
	ch = bytes = 0;
	line = calloc(sz, sizeof(char));

	if (line == NULL) {
		perror("calloc");
		return;
	}

	while (read(fd, &ch, 1) == 1) {
		if ((bytes + 1) >= sz) {
			sz *= 2;
			tmpbuf = realloc(line, sz * sizeof(char));
			
			if (tmpbuf == NULL) {
				perror("realloc");
				free(line);
				return;
			}

			line = tmpbuf;
		}

		line[bytes++] = ch;
	}

	line[bytes] = 0;
	tok = strtok(line, "\r\n");

	while (tok != NULL) {
		arg = parse_cmd(tok);
		if (strcasecmp(arg.argv[0], "exit") == 0)
			break;
		dos_exec(arg.argv[0], &arg.argv[1],
		         arg.argc - 1, true);
		tok = strtok(NULL, "\r\n");
	}

	free(line);
}
