#include "headers/dos_lib.h"
#include "headers/dos_const.h"
#include "headers/print.h"
#include "headers/conio.h"
#include "headers/parse_opt.h"
#include "headers/dos_exec.h"
#include "headers/trim.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

HANDLE *handles;
BYTE *MEMORY;
int ERRORLEVEL = 0;
BOOL memory_freeable = false;
BOOL handles_freeable = false;
label *labels;
size_t labels_n, lbl_cnt;

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

	if (res8 == 0)                         r->flags |= ZF;
	if (res > 0xFF)                        r->flags |= CF; 
	if (res8 & 0x80)                       r->flags |= SF;
	if (bitsnum(res8) % 2 == 0)            r->flags |= PF;
	if (((dst & 0xF) + (src & 0xF)) > 0xF) r->flags |= AF;
	if (((dst & 0x80) == (src & 0x80))
	   &&(dst & 0x80) != (res8 & 0x80))    r->flags |= OF;
}

void
update_f16_incdec (REGS *r, WORD old_v, WORD res, BOOL is_inc)
{
	r->flags &= ~(CF | OF | PF | AF | ZF | SF);

	if (res == 0)                          r->flags |= ZF;
	if (res & 0x8000)                      r->flags |= SF;
	if (bitsnum(res & 0xFF) % 2 == 0)      r->flags |= PF;
	if (is_inc) {
		if ((old_v & 0xF) + 1 > 0xF)   r->flags |= AF;
		if (old_v == 0x7FFF)           r->flags |= OF;
	}
	else {
		if ((old_v & 0xF) - 1 < 0)     r->flags |= AF;
		if (old_v == 0x8000)           r->flags |= OF;
	}
}

void
update_f16 (REGS *r, WORD dst, WORD src, DWORD res)
{
	r->flags &= ~(CF | OF | PF | AF | ZF | SF);

	if (res == 0)                              r->flags |= ZF;
	if (res > 0xFFFF)                          r->flags |= CF; 
	if (res & 0x8000)                          r->flags |= SF;
	if (bitsnum(res & 0xFF) % 2 == 0)          r->flags |= PF;
	if (((dst & 0xF) + (src & 0xF)) > 0xF)     r->flags |= AF;
	if (((dst & 0x8000) == (src & 0x8000))
	   &&(dst & 0x8000) != (res & 0x8000))     r->flags |= OF;
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

DWORD
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
		if (rm == 6) {
			addr = (MEMORY[(*ipidx)])
			     | (MEMORY[(*ipidx) + 1] << 8);
			(*ipidx) += 2;
			addr = (WORD)SEG_OFF(r->DS, addr);
			return (WORD)addr;
		}
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
	default: return (DWORD)-1;
	}

	addr += disp;
	addr = SEG_OFF(r->DS, addr);

	return (WORD)addr;
}

void
exec_add_r8 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm, *dst, *src, src_val, dst_val;
	WORD res;
	DWORD addr;
	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;
	src   = get_r8(r, reg);
	src_val = *src;

	if (mod == 3) {
		dst = get_r8(r, rm);
		dst_val = *dst;
		res = dst_val + src_val;
		*dst = res & 0xFF;

		update_f8(r, dst_val, src_val, res);
	}
	else {
		addr = calc_ea(r, modrm, ipidx);

		if (addr == (DWORD)-1 && modrm != 6) {
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
exec_mov_r16 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm;
	WORD *dst, *src, val;
	DWORD addr;

	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;
	dst   = get_r16(r, reg);

	if (mod == 3) {
		src = get_r16(r, rm);
		*dst = *src;
	}
	else {
		addr = calc_ea(r, modrm, ipidx);

		if (addr == (DWORD)-1 && modrm != 6) {
			puts("Failed to calculate effective address");
			return;
		}

		val = MEMORY[addr] | (MEMORY[addr + 1] << 8);
		*dst = val;
	}
}

void
exec_mov_r8 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm, *dst, *src;
	DWORD addr;

	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;
	dst   = get_r8(r, reg);

	if (mod == 3) {
		src = get_r8(r, rm);
		*dst = *src;
	}
	else {
		addr = calc_ea(r, modrm, ipidx);

		if (addr == (DWORD)-1 && modrm != 6) {
			puts("Failed to calculate effective address");
			return;
		}

		*dst = MEMORY[addr];
	}
}

void
exec_80 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm, val, *r8, imm;
	WORD addr, res;

	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;

	switch (reg) {
	/* cmp */
	case 7:
		if (mod == 3) {
			r8 = get_r8(r, rm);
			val = *r8;
		}
		else {
			addr = calc_ea(r, modrm, ipidx);
			val = MEMORY[addr];
		}

		imm = MEMORY[(*ipidx)++];
		res = val - imm;

		update_f8(r, val, imm, res);

		break;
	}
}

void
exec_83 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm;
	int8_t imm8;
	DWORD addr;
	WORD *r16, val, imm16, res;
	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;

	switch (reg) {
	/* cmp */
	case 7:
		if (mod == 3) {
			r16 = get_r16(r, rm);
			val = *r16;
		}
		else {
			addr = calc_ea(r, modrm, ipidx);
			val  = (MEMORY[addr])
			     | (MEMORY[addr + 1] << 8);
		}

		imm8  = MEMORY[(*ipidx)++];
		imm16 = (WORD)(int8_t)imm8;
		res = val - imm16;

		update_f16(r, val, imm16, res);

		break;
	}
}

void
exec_f6 (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm, val, *r8;
	WORD addr, acpy;

	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;

	switch (reg) {
	case 6:
		if (mod == 3) {
			r8 = get_r8(r, rm);
			val = *r8;
		}
		else {
			addr = calc_ea(r, modrm, ipidx);
			val = MEMORY[addr];
		}

		acpy = r->AH;
		r->AL = acpy / val;
		r->AH = acpy % val;

		break;
	}
}

void
exec_ff (REGS *r, WORD *ipidx)
{
	BYTE modrm, mod, reg, rm;
	DWORD addr;
	WORD *r16, val;

	modrm = MEMORY[(*ipidx)++];
	mod   = (modrm >> 6) & 0x03;
	reg   = (modrm >> 3) & 0x07;
	rm    = modrm & 0x07;

	switch (reg) {
	case 0:
		if (mod == 3) {
			r16 = get_r16(r, rm);
			(*r16)++;

			update_f16_incdec(r, (*r16) - 1, *r16, true);
		}
		else {
			addr = calc_ea(r, modrm, ipidx);

			if (addr == (DWORD)-1 && modrm != 6) {
				puts("Failed to calculate effective address");
				return;
			}

			val = (MEMORY[addr])
			    | (MEMORY[addr + 1] << 8);
			val++;

			update_f16_incdec(r, val - 1, val, true);

			MEMORY[addr]     = val & 0xFF;
			MEMORY[addr + 1] = (val >> 8) & 0xFF;
		}
		break;
	case 1:
		if (mod == 3) {
			r16 = get_r16(r, rm);
			(*r16)--;

			update_f16_incdec(r, (*r16) + 1, *r16, true);
		}
		else {
			addr = calc_ea(r, modrm, ipidx);

			if (addr == (DWORD)-1 && modrm != 6) {
				puts("Failed to calculate effective address");
				return;
			}

			val = (MEMORY[addr])
			    | (MEMORY[addr + 1] << 8);
			val--;

			update_f16_incdec(r, val + 1, val, false);

			MEMORY[addr]     = val & 0xFF;
			MEMORY[addr + 1] = (val >> 8) & 0xFF;
		}
		break;
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
	char *line;
	size_t bytes_read;
	addr = SEG_OFF(r->DS, r->DX);
	max_chars = MEMORY[addr];
	
	rl_bind_keyseq("\033[A", NULL);
	rl_bind_keyseq("\033[B", NULL);

	line = readline(" ");

	rl_bind_keyseq("\033[A", rl_get_previous_history);
	rl_bind_keyseq("\033[B", rl_get_next_history);
	
	if (line == NULL) {
		r->AL = 0;
		MEMORY[addr + 1] = 0;

		perror("malloc");

		return;
	}

	bytes_read = strlen(line);

	if (bytes_read < 1) {
		r->AL = 0;
		MEMORY[addr + 1] = 0;

		free(line);

		return;
	}

	if (bytes_read > max_chars) bytes_read = max_chars;

	memcpy(&MEMORY[addr + 2], line, bytes_read);
	free(line);

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
int21h (REGS *r)
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
	BYTE ch, ch2, mod, reg, rm, *src, *dst, off, modrm;
	WORD *ipidx, off16, seg16, res, val, addr;
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
		/* cmp $imm/r8 %r8 */
		case 0x38:
			modrm = MEMORY[(*ipidx)++];
			mod   = (modrm >> 6) & 0x3;
			reg   = (modrm >> 3) & 0x7;
			rm    = modrm & 0x7;

			if (mod == 3) {
				dst = get_r8(r, rm);
				src = get_r8(r, reg);
				val = *dst;
			}
			else {
				addr = calc_ea(r, modrm, ipidx);
				val = MEMORY[addr];
				src = get_r8(r, reg);
			}

			res = val - *src;

			update_f8(r, val, *src, res);
			break;
		/* cmp %al, $imm8 */
		case 0x3C:
			val = MEMORY[(*ipidx)++];
			res = r->AL - val;

			update_f8(r, r->AL, val, res);
			break;
		/* jo $imm8 */
		case 0x70:
			off = MEMORY[(*ipidx)++];

			if (r->flags & OF)
				(*ipidx) += (signed char)off;
			break;
		/* jno $imm8 */
		case 0x71:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & OF))
				(*ipidx) += (signed char)off;
			break;
		/* jb/jnae/jc $imm8 */
		case 0x72:
			off = MEMORY[(*ipidx)++];

			if (r->flags & CF)
				(*ipidx) += (signed char)off;
			break;
		/* jnb/jae/jnc $imm8 */
		case 0x73:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & CF))
				(*ipidx) += (signed char)off;
			break;
		/* je/jz $imm8 */
		case 0x74:
			off = MEMORY[(*ipidx)++];

			if (r->flags & ZF)
				(*ipidx) += (signed char)off;
			break;
		/* jne/jnz $imm8 */
		case 0x75:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & ZF))
				(*ipidx) += (signed char)off;
			break;
		/* jbe/jna $imm8 */
		case 0x76:
			off = MEMORY[(*ipidx)++];

			if (r->flags & ZF || r->flags & CF)
				(*ipidx) += (signed char)off;
			break;
		/* jnbe/ja $imm8 */
		case 0x77:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & ZF) && !(r->flags & CF))
				(*ipidx) += (signed char)off;
			break;
		/* js $imm8 */
		case 0x78:
			off = MEMORY[(*ipidx)++];

			if (r->flags & SF)
				(*ipidx) += (signed char)off;
			break;
		/* jns $imm8 */
		case 0x79:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & SF))
				(*ipidx) += (signed char)off;
			break;
		/* jp/jpe $imm8 */
		case 0x7a:
			off = MEMORY[(*ipidx)++];

			if (r->flags & PF)
				(*ipidx) += (signed char)off;
			break;
		/* jnp/jpo $imm8 */
		case 0x7b:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & PF))
				(*ipidx) += (signed char)off;
			break;
		/* jl/jnge $imm8 */
		case 0x7c:
			off = MEMORY[(*ipidx)++];

			if ((r->flags & SF) != (r->flags & OF))
				(*ipidx) += (signed char)off;
			break;
		/* jnl/jge $imm8 */
		case 0x7d:
			off = MEMORY[(*ipidx)++];

			if ((r->flags & SF) == (r->flags & OF))
				(*ipidx) += (signed char)off;
			break;
		/* jle/jng $imm8 */
		case 0x7e:
			off = MEMORY[(*ipidx)++];

			if ((r->flags & ZF) || ((r->flags & SF)
			 != (r->flags & OF)))
				(*ipidx) += (signed char)off;
			break;
		/* jnle/jg $imm8 */
		case 0x7f:
			off = MEMORY[(*ipidx)++];

			if (!(r->flags & ZF) && ((r->flags & SF)
			  == (r->flags & OF)))
				(*ipidx) += (signed char)off;
			break;
		case 0x80:
			exec_80(r, ipidx);
			break;
		case 0x83:
			exec_83(r, ipidx);
			break;
		/* mov %r8, %r8 */
		case 0x88:
			mod   = MEMORY[(*ipidx)++];
			reg   = (mod >> 3) & 0x7;
			rm    = mod & 0x7;
			src   = get_r8(r, reg);
			dst   = get_r8(r, rm);
			*dst = *src;
			break;
		/* mov %r8, $imm/r8 */
		case 0x8A:
			exec_mov_r8(r, ipidx);
			break;
		/* mov %r16, $imm/r16*/
		case 0x8B:
			exec_mov_r16(r, ipidx);
			break;
		/* nop */
		case 0x90:
			(*ipidx)++;
			break;
		/* mov $moffs8, %al */
		case 0xA0:
			off16 = MEMORY[(*ipidx)] | (MEMORY[(*ipidx) + 1] << 8);
			r->AL = MEMORY[off16];
			(*ipidx) += 2;
			break;
		/* mov $imm8, %cl */
		case 0xB1:
			r->CL = MEMORY[(*ipidx)++];
			break;
		/* mov $imm8, %dl */
		case 0xB2:
			r->DL = MEMORY[(*ipidx)++];
			break;
		/* mov $imm8, %bl */
		case 0xB3:
			r->BL = MEMORY[(*ipidx)++];
			break;
		/* mov $imm8, %ah */
		case 0xB4:
			r->AH = MEMORY[(*ipidx)++];
			break;
		/* mov $imm16, %dx */
		case 0xBA:
			r->DX = MEMORY[(*ipidx)] | (MEMORY[(*ipidx) + 1] << 8);
			(*ipidx) += 2;
			break;
		/* int $imm8 */
		case 0xCD:
			ch2 = MEMORY[(*ipidx)++];
			switch (ch2) {
			case 0x20:
				goto unload_com;
			case 0x21:
				if (r->AH == 0x4C) goto unload_com;
				int21h(r);
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
				goto unload_com;
			}

			newip = MEMORY[r->SP] | (MEMORY[r->SP + 1] << 8);
			r->SP += 2;
			r->IP = newip;
			(*ipidx) = r->IP;

			goto unload_com;
		}
		/* jmp $imm8 */
		case 0xEB:
			off = MEMORY[(*ipidx)++];
			(*ipidx) += (WORD)(int8_t)off;
			break;
		/* jmp $imm16 */
		case 0xE9:
			off16 = (MEMORY[(*ipidx)])
			      | (MEMORY[(*ipidx) + 1] << 8);
			(*ipidx) += 2;
			(*ipidx) += (int16_t)off16;
			break;
		/* jmp far $imm16:imm16 */
		case 0xEA:
			off16 = (MEMORY[(*ipidx)])
			      | (MEMORY[(*ipidx) + 1] << 8);
			seg16 = (MEMORY[(*ipidx) + 2])
			      | (MEMORY[(*ipidx) + 3] << 8);
			r->CS = seg16;
			r->IP = off16;
			(*ipidx) = SEG_OFF(seg16, off16);
			break;
		case 0xF6:
			exec_f6(r, ipidx);
			break;
		case 0xFF:
			exec_ff(r, ipidx);
			break;
		default:
			printf("Unhandled opcode: 0x%02X", ch);
			goto unload_com;
		}
	}

	unload_com:
	memset(&MEMORY[PRG_START], 0, MEM_MAX - PRG_START);
}

size_t
find_label (char *label)
{
	size_t i;

	for (i = 0; i < lbl_cnt; i++) {
		if (strcasecmp(labels[i].name, label) == 0) {
			return labels[i].line;
		}
	}

	return (size_t)-1;
}

bool
label_exists (char *label)
{
	size_t i;

	for (i = 0; i < lbl_cnt; i++) {
		if (strcasecmp(labels[i].name, label) == 0)
			return true;
	}

	return false;
}

void
addlabel (char *label_name, size_t linenum)
{
	size_t lbl_line;
	label *tmp;
	lbl_line = lbl_cnt;

	if (label_exists(label_name))
		lbl_line = find_label(label_name);
	if (strlen(label_name) >= LABELN_MAX) {
		printf("Label name exceeds %d"
		       " character limit.", LABELN_MAX);

		return;
	}
	if (lbl_cnt >= labels_n) {
		labels_n *= 2;
		tmp = realloc(labels,
		(size_t)labels_n * sizeof(*labels));

		if (tmp == NULL) {
			perror("realloc");
			return;
		}

		labels = tmp;
	}

	labels[lbl_line].line = linenum;

	snprintf(labels[lbl_line].name,
		 sizeof labels[lbl_line].name,
		 "%s", label_name);

	if (lbl_line == lbl_cnt) lbl_cnt++;
}

void
init_labels (char **file, size_t lines)
{
	char *labeln;
	size_t i;

	for (i = 0; i < lines; i++) {
		if (file[i][0] == ':') {
			labeln = file[i] + 1;

			trimr(labeln, NULL);
			addlabel(labeln, i);
		}
	}
}

void
runbat (int fd)
{
	char *line, *tmpbuf, *tok, *cmd, *savptr = NULL,
	     **lines, **tmp, *path;
	int ch, bytes, i;;
	size_t sz, rgoto, linenum, lines_max, lines_n;
	struct opt arg;
	bool local_echo;
	sz = labels_n = lines_max = 256;
	ch = bytes = lbl_cnt = lines_n = linenum = 0;
	line = calloc(sz, sizeof(char));
	labels = calloc(labels_n, sizeof *labels);
	lines = calloc(lines_max, sizeof(char *));
	local_echo = true;

	if (line == NULL || labels == NULL || lines == NULL) {
		perror("calloc");
		return;
	}

	while (read(fd, &ch, 1) == 1) {
		if ((size_t)(bytes + 1) >= sz) {
			sz *= 2;
			tmpbuf = realloc(line, (long unsigned int)sz
			                       * sizeof(char));
			
			if (tmpbuf == NULL) {
				perror("realloc");
				free(line);
				return;
			}

			line = tmpbuf;
		}

		line[bytes++] = (char)ch;
	}

	line[bytes] = 0;
	tok = strtok_r(line, "\r\n",  &savptr);

	while (tok != NULL) {
		if (lines_n >= lines_max) {
			lines_max *= 2;
			tmp = realloc(lines, lines_max * sizeof(char *));

			if (tmp == NULL) {
				perror("realloc");
				free(line);
				free(labels);

				return;
			}

			lines = tmp;
		}

		lines[lines_n++] = tok;
		tok = strtok_r(NULL, "\r\n", &savptr);
	}

	init_labels(lines, lines_n);

	while (linenum < lines_n) {
		tok = lines[linenum];
		arg = parse_cmd(tok);

		if (arg.argv == NULL) {
			tok = strtok_r(NULL, "\r\n", &savptr);
			linenum++;
			continue;
		}
		if (arg.argc < 1) goto next;

		cmd = arg.argv[0];

		if (cmd[0] == ':')
			goto next;
		if (cmd[0] == '@')
			cmd++;
		if ((strcasecmp(cmd, "echo") == 0
		&& (arg.argc > 1))
		&& ((strcasecmp(arg.argv[1], "on") == 0)
		|| (strcasecmp(arg.argv[1], "off") == 0))) {
			if (strcasecmp(arg.argv[1], "on") == 0)
				local_echo = true;
			else
				local_echo = false;
			goto next;
		}
		if (strcasecmp(cmd, "goto") == 0) {
			if (arg.argc < 2) {
				puts("GOTO requires label name");
				goto next;
			}
			if (arg.argc > 2) {
				puts(DOSSTR_ILLEGAL_SYN);
				goto next;
			}

			rgoto = find_label(arg.argv[1]);

			if (rgoto == (size_t)-1) {
				puts(arg.argv[1]);
				puts("GOTO label not found");
				goto next;
			}

			linenum = rgoto;

			goto next;
		}
		if (local_echo && strlen(cmd) > 0
		&& (strcasecmp(cmd, "rem") != 0)) {
			path = get_path();

			if (path == NULL)
				goto next;

			print(path);

			for (i = 0; i < arg.argc; i++)
				printf("%s ", arg.argv[i]);

			putchar('\n');
		}
		if (strcasecmp(cmd, "exit") == 0)
			break;

		dos_exec(cmd, &arg.argv[1],
		         arg.argc - 1, true);

		next:
		for (i = 0; i < arg.argc; i++)
			free(arg.argv[i]);
		free(arg.argv);

		linenum++;
	}

	free(lines);
	free(labels);
	free(line);
}
