; read input and print it

ORG 100h

SECTION .data
	prompt db "Enter text: $"
	output db "You wrote: $"
	newline db 0Dh, 0Ah, '$'
	buffer db 31, 0, 32 DUP('$')

SECTION .text
START:
	mov dx, prompt
	mov ah, 09h
	int 21h 

	mov dx, buffer
	mov ah, 0Ah
	int 21h

	mov dx, newline
	mov ah, 09h
	int 21h

	mov dx, output
	mov ah, 09h
	int 21h

	mov dx, buffer + 2
	mov ah, 09h
	int 21h

	mov dx, newline
	mov ah, 09h
	int 21h

	ret
