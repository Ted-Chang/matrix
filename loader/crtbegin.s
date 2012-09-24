;
; crtbegin.s
;
BITS 32

; defined in applications
extern main

global _start
_start:				; Global entry point
	pop eax			; Our stack is slightly off
	call main		; Call C main function
	mov ebx, eax		; Return value from main
	mov eax, 0x0		; sys_exit
	int 0x80		; syscall
_wait:				; Wait until we've been deschuled
	hlt
	jmp _wait