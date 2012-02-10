	;;
	;; boot.s -- Kernel start location. Also defines multiboot header.
	;;

	[BITS 32]		; All instructions should be 32-bit

	[GLOBAL mboot]		; Make 'mboot' accessible from C.
	[EXTERN code]		; Start of the '.text' section.
	[EXTERN bss]		; Start of the '.bss' section.
	[EXTERN end]		; End of the last loadable section.

mboot:
	dd MBOOT_HEADER_MAGIC	; GRUB will search for this magic
				; 4-byte boundary in your kernel file
	dd MBOOT_HEADER_FLAGS	; How GRUB should load your file/settings
	dd MBOOT_CHECKSUM	; To ensure that the above magics are correct

	dd mboot		; Location of this descriptor
	dd code			; Start of kernel '.text' (code) section.
	dd bss			; End of kernel '.data' section
	dd end			; End of kernel.
	dd start		; Kernel entry point (initial EIP).

	[GLOBAL start]		; Kernel entry point.
	[EXTERN main]		; This is the entry point of our C code

start:
	push	ebx		; Load multiboot header location

	;; Execute our kernel
	cli
	call main
	jmp $