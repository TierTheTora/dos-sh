; rock paper scissors

ORG 100h

START:
	mov ah, [46Ch]
	mov cl, 3
	div cl

	cmp ah, 0
	je rock
	cmp ah, 1
	je paper
	cmp ah, 2
	je scissor

	rock:
	mov bl, 'r'
	jmp game

	paper:
	mov bl, 'p'
	jmp game

	scissor:
	mov bl, 's'
	jmp game

	game:

	mov ah, 01h
	int 21h

	cmp al, bl
	je draw
	cmp al, 'r'
	je player_r
	cmp al, 'p'
	je player_p
	cmp al, 's'
	je player_s
	mov ah, 09h
	mov dx, invalid
	int 21h
	jmp START

	player_r:
	mov ah, 02h
	mov dl, bl
	int 21h
	mov ah, 09h
	mov dx, lf
	int 21h

	cmp bl, 's'
	je win
	jmp lose

	player_p:
	mov ah, 02h
	mov dl, bl
	int 21h
	mov ah, 09h
	mov dx, lf
	int 21h

	cmp bl, 'r'
	je win
	jmp lose

	player_s:
	mov ah, 02h
	mov dl, bl
	int 21h

	mov ah, 09h
	mov dx, lf
	int 21h
	cmp bl, 'p'
	je win
	jmp lose

	draw:
	mov ah, 02h
	mov dl, bl
	int 21h
	mov ah, 09h
	mov dx, lf
	int 21h

	mov ah, 09h
	mov dx, drawstr
	int 21h
	jmp exit

	win:
	mov ah, 09h
	mov dx, winstr
	int 21h
	jmp exit

	lose:
	mov ah, 09h
	mov dx, losestr
	int 21h

	exit:
	ret

SECTION .data
	drawstr db "Draw!", 0Dh, 0Ah, '$'
	winstr  db "Win!", 0Dh, 0Ah, '$'
	losestr db "Lose!", 0Dh, 0Ah, '$'
	invalid db "Pick 'r', 'p' or 's'.", 0Dh, 0Ah, '$'
	lf db 0Dh, 0Ah, '$'
