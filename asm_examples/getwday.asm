; get week day

ORG 100h

START:
	mov ah, 24h
	int 21h

	mov ah, 09h
	mov dx, str_todayis
	int 21h

	cmp al, 0
	je case_sunday
	cmp al, 1
	je case_monday
	cmp al, 2
	je case_tuesday
	cmp al, 3
	je case_wednesday
	cmp al, 4
	je case_thursday
	cmp al, 5
	je case_friday
	cmp al, 6
	je case_saturday
	jmp _default

	case_sunday:
	mov dx, str_sunday
	jmp continue

	case_monday:
	mov dx, str_monday
	jmp continue

	case_tuesday:
	mov dx, str_tuesday
	jmp continue

	case_wednesday:
	mov dx, str_wednesday
	jmp continue

	case_thursday:
	mov dx, str_thursday
	jmp continue

	case_friday:
	mov dx, str_friday
	jmp continue

	case_saturday:
	mov dx, str_saturday
	jmp continue

	_default:
	mov dx, str_default

	continue:
	int 21h
	ret

SECTION .data
	str_todayis	db "Today is... $"
	str_sunday	db "Sunday!",			0Dh, 0Ah, '$'
	str_monday	db "Monday!",			0Dh, 0Ah, '$'
	str_tuesday	db "Tuesday!",			0Dh, 0Ah, '$'
	str_wednesday	db "Wednesday!",		0Dh, 0Ah, '$'
	str_thursday	db "Thursday!",			0Dh, 0Ah, '$'
	str_friday	db "Friday!",			0Dh, 0Ah, '$'
	str_saturday	db "Saturday!",			0Dh, 0Ah, '$'
	str_default	db "Not sure..$",		0Dh, 0Ah, '$'
