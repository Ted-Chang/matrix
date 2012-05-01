;
; process.s
; 
[GLOBAL read_eip]
read_eip:			; When read_eip is called, the current instruction location
				; is pushed on to the stack, we pop it to EAX and return.
	pop eax			; Get the instruction pointer
	jmp eax			; Return. Can't use RET because return
				; address popped off the stack


