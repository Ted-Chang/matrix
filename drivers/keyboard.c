#include <types.h>
#include "hal.h"
#include "util.h"
#include "isr.h"
#include "keyboard.h"

static void keyboard_callback(struct registers *regs)
{
	u_char scan_code;
	u_long temp;

	scan_code = inportb(0x60);

	/* Do something with the scan code.
	 * Remember you only get `one' byte of the scan code each time the
	 * ISR is invoked.
	 */
	kprintf("keyboard_callback: scan_code(0x%x).\n", scan_code);
}

void init_keyboard()
{
	register_interrupt_handler(IRQ1, keyboard_callback);
}
