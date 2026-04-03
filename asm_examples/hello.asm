; print "Hello, World!"

ORG 100h

SECTION .data
	hellomsg db "Hello, World!", 0Dh, 0Ah, '$'

SECTION .text

START:
	mov dx, hellomsg
	mov ah, 09h
	int 21h

	ret
