/*
 * Userspace Application Linker Script
 * 2012, Ted Chang
 */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
STARTUP(../bin/lib/crt1.o)
INPUT(../bin/lib/libinit.o)
INPUT(../bin/lib/syscalls.o)
INPUT(../bin/lib/div64.o)
INPUT(../bin/lib/string.o)
INPUT(../bin/lib/vsprintf.o)
INPUT(../bin/lib/stdio.o)
INPUT(../bin/lib/printf.o)
INPUT(../bin/lib/intrnl.o)
phys = 0x20000000;
SECTIONS
{
	/*
	 * Actual code
	 */
	.text phys : AT(phys) {
		code = .;
		*(.text)
		*(.rodata)
		. = ALIGN(4096);
	}
	/*
	 * data
	 */
	.data : AT(phys + (data - code))
	{
		data = .;
		*(.data)
		. = ALIGN(4096);
	}
	/*
	 * Statically defined, uninitialized values
	 */
	.bss : AT(phys + (bss - code))
	{
		bss = .;
		*(.bss)
		. = ALIGN(4096);
	}
	/*
	 * Get rid of unnecessary GCC bits.
	 */
	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.note.gnu.build-id)
	}
}