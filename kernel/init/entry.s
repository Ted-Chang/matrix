;
; boot.s -- Kernel start location. Also defines multiboot header.
;

; Multiboot constant definitions
; Load kernel and modules on a page boundary
MBOOT_PAGE_ALIGN	equ 1<<0
; Provide your kernel with memory info
MBOOT_MEM_INFO		equ 1<<1
; Multiboot magic number
MBOOT_HEADER_MAGIC	equ 0x1BADB002

; NOTE: We do not use MBOOT_AOUT_KLUDGE. It means that GRUB does
; not pass a symbol table to us.
MBOOT_HEADER_FLAGS	equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM		equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

section .__mbHeader
align 4
mboot:
	dd MBOOT_HEADER_MAGIC	; GRUB will search for this magic
				; 4-byte boundary in your kernel file
	dd MBOOT_HEADER_FLAGS	; How GRUB should load your file/settings
	dd MBOOT_CHECKSUM	; To ensure that the above magics are correct


[BITS 32]			; All instructions should be 32-bit

[EXTERN code]			; Start of the '.text' section.
[EXTERN bss]			; Start of the '.bss' section.
[EXTERN end]			; End of the last loadable section.

[GLOBAL start]			; Kernel entry point.
[EXTERN kmain]			; The entry point of our C code
start:
	push esp		; We need to know exactly where the current stack starts
	push ebx		; Load multiboot header location

	;; Execute our kernel defined in main.c
	cli
	call kmain
	jmp $