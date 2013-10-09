/*
 * Link.ld -- Linker script for the matrix kernel - ensure everything goes
 *            in the correct place.
 *            Original file taken from www.jamesmolloy.co.uk
 */

ENTRY(start)
SECTIONS
{
	/*
	 * Told the linker to place the __mbHeader section at 0x100000,
	 * the original way doesn't work when the kernel image grows too
	 * large. So we place a separate section here to guarantee the
	 * multiboot header will be in the 1st 8KB of the kernel image.
	 */
	.__mbHeader 0x100000: 
	{
		*(.__mbHeader)
	}

	.text :
	{
		code = .; _code = .; __code = .;
		*(.text)
		. = ALIGN(0x1000);
	}

	.data :
	{
		data = .; _data = .; __data = .;
		*(.data)
		*(.rodata)
		. = ALIGN(0x1000);
	}

	.bss :
	{
		bss = .; _bss = .; __bss = .;
		*(.bss)
		. = ALIGN(0x1000);
	}

	.init :
	{
		init = .; _init = .; __init = .;
		__ac_trampoline_start = .;
		*(.__ac_init_trampoline)
		__ac_trampoline_end = .;
		. = ALIGN(0x1000);
	}

	end = .; _end = .; __end = .;
}