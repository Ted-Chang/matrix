	;;
	;; CRT library
	;; 
BITS 32
extern main
global _start
_start:				; Global entry point
	pop eax			; Our stack is slightly off
	call main		; Call C main function
	mov ebx, eax		; Return value from main
	mov eax, 0x5		; System call exit
	int 0x80		; Make system call
_wait:
	hlt
	jmp _wait