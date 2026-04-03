; print every byte in memory

ORG 100h

SECTION .data
	addr dw 0

SECTION .text

START:
loop:
	mov si, [addr]
	mov dl, [si]
	mov ah, 02h
	int 21h
	inc word [addr]
	cmp word [addr], 0xFFFF
	jnz loop
	ret
