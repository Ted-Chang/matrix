;
; process.s
; 
[GLOBAL read_eip]
read_eip:			; When read_eip is called, the current instruction location
				; is pushed on to the stack, we pop it to EAX and return.
	pop eax			; Get the instruction pointer
	jmp eax			; Return. Can't use RET because return
				; address popped off the stack

[GLOBAL x86_context_switch]
x86_context_switch:
	push ebx		; According to __cdecl convention, we must preserve the contents
	pushf			; of EBX and push EFLAGS so we can pop it and reenable interrupts

	push ebp		; Save ebp to current stack
	mov [esp+12], esp	; Save esp to current stack
	
	mov esp, [esp+16]	; Previous process
	pop ebp			; Restore ebp
	
	popf			; Pop EFLAGS back
	pop ebx			; Get original EBX value back
	ret


[GLOBAL x86_context_restore]
x86_context_restore:
	push ebx
	pushf

	mov esp, [esp+12]	; Set stack pointer to current process
	pop ebp			; Restore ebp
	
	popf
	pop ebx
	ret


[GLOBAL copy_page_physical]
copy_page_physical:
	push ebx		; According to __cdecl, we must preserve the contents of EBX
	pushf			; push EFLAGS, so we can pop it and reenable interrupts
				; later, if they were enabled anyway.

	cli			; Disable interrupts, so we aren't interrupted

	mov ecx, [esp+12]	; Destination address
	mov ebx, [esp+16]	; Source address

	mov edx, cr0		; Get the control register
	and edx, 0x7FFFFFFF	; and
	mov cr0, edx		; Disable paging

	mov edx, 1024		; 1024*4bytes = 4096 bytes to copy

.loop:
	mov eax, [ebx]		; Get the word at the source address
	mov [ecx], eax		; Store it at the destination address
	add ebx, 4		; Source address += sizeof(word)
	add ecx, 4		; Destination address += sizeof(word)
	dec edx			; One less word to do
	jnz .loop

	mov edx, cr0		; Get the control register again
	or edx, 0x80000000	; and
	mov cr0, edx		; Enable paging

	popf			; Pop EFLAGS back
	pop ebx			; Get the original value of EBX back
	ret