; read one char and print it

ORG 100h

SECTION .text

START:
	mov ah, 01h
	int 21h
	mov dl, al
	mov ah, 02h
	int 21h
	ret
