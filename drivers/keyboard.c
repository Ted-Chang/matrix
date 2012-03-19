#include <types.h>
#include "hal.h"
#include "util.h"
#include "isr.h"
#include "keyboard.h"

struct irq_hook _kbd_hook;

static char _scan_code;

static boolean_t _numlock;
static boolean_t _scrolllock;
static boolean_t _capslock;

static boolean_t _shitf;
static boolean_t _alt;
static boolean_t _ctrl;

static void kbd_callback(struct registers *regs)
{
	u_char scan_code;
	u_long temp;

	scan_code = inportb(0x60);

	/* Do something with the scan code.
	 * Remember you only get `one' byte of the scan code each time the
	 * ISR is invoked.
	 */
	kprintf("kbd_callback: scan_code(0x%x).\n", scan_code);
}

void init_keyboard()
{
	_scan_code = 0;
	_numlock = _scrolllock = _capslock = FALSE;
	_shitf = _alt = _ctrl = FALSE;
	
	register_interrupt_handler(IRQ1, &_kbd_hook, kbd_callback);
}
