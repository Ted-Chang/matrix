	;;
	;; CRT library
	;; 
BITS 32
extern libc_main
global _start
_start:				; Global entry point
	call libc_main		; Call libc_main function
_wait:
	hlt
	jmp _wait