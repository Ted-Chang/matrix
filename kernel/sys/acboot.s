;
; acboot.s -- Application core boot code
;

[BITS 16]			; The boot instruction for this phase is 16bit

section	.__ac_init_trampoline
[GLOBAL acstart]
acstart:
	jmp	ac_boot		; Make the jump
	
align 	16
; The following are the arguments need to beset by the caller
entry_addr	dd 0
entry_arg	dd 0		; This point to a core structure
kernel_sp	dd 0
kernel_cr3	dd 0

align 	8
ac_boot:	
	xor	eax, eax
	
	;; Set the data segment
	mov 	ax, cs
	mov 	ds, ax
	mov	es, ax
	mov	fs, ax

	;; Following is the debug code, remove it when you are able to go on
	jmp	$

	;; Set the correct base address of the GDT and load it
	mov	eax, _base
	mov	dword [eax], _gdt
	mov	eax, _gdtptr
	lgdt	[eax]

	;; Switch protect mode
	mov	eax, cr0
	or	eax, 0x01
	mov	cr0, eax

	;; Jump to the 32-bit code segment, this is a far jump!
	jmp	dword 0x08:.ac_boot32

align 	8
[BITS 32]
.ac_boot32:
	;; Load data segment
	mov 	ax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	fs, ax

	;; Load the stack address
	mov	esp, [kernel_sp]

	;; Clear the stack frame/FLAGS
	xor	ebp, ebp

	;; Call the AC kernel entry (kmain_ac)
	call	[entry_addr]
	jmp	$

_gdt:
	dq	0x0000000000000000 	; NULL descriptor
	dq	0x00CF9A000000FFFF 	; Kernel code segment
	dq	0x00CF92000000FFFF 	; Kernel data segment
	dq	0x00CFFA000000FFFF 	; Usermode code segment
	dq	0x00CFF2000000FFFF 	; Usermode data segment
_gdtend:	
_gdtptr:
_limit 	dw	_gdtend - _gdt - 1	; sizeof gdt - 1
					; you add or remove entry in gdt
_base	dd	0			; Base address will be update at run time