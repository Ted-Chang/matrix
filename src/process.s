[GLOBAL read_eip]
read_eip:
	pop eax			; Get the return address
	jmp eax			; Return. Can't use RET because return
				; address popped off the stack

[GLOBAL copy_page_physical]
copy_page_physical:
	push ebx
	pushf

	cli

	mov ebx, [esp+12]
	mov ecx, [esp+16]

	mov edx, cr0
	and edx, 0x7FFFFFFF
	mov cr0, edx

	mov edx, 1024

.loop:
	mov eax, [ebx]
	mov [ecx], eax
	add ebx, 4
	add ecx, 4
	dec edx
	jnz .loop

	mov edx, cr0
	or edx, 0x80000000
	mov cr0, edx

	popf
	pop ebx
	ret