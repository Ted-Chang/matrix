#include <types.h>
#include "hal.h"
#include "util.h"
#include "isr.h"
#include "keyboard.h"

struct key_code {
	struct {
		uint8_t data_len:3;
		uint8_t reserved:1;
		uint8_t lock:1;
		uint8_t numlock_af:1;
		uint8_t capslock_af:1;
		uint8_t s_flag:1;
	} flags;

	uint8_t cmd_bits;
	uint16_t data[4];	// A variable data
};

struct irq_hook _kbd_hook;

static char _scan_code;

static boolean_t _numlock;
static boolean_t _scrolllock;
static boolean_t _capslock;

static boolean_t _shitf;
static boolean_t _alt;
static boolean_t _ctrl;

const struct key_code _kmap[128] = {
	/* Flags,   CMD               N,  SHIFT,   CTRL,    ALT */
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},
	{{3, 0, 0, 0, 0, 1}, 0, {0x011B, 0x011B, 0x011B, 0x0100}},	// escape
	{{3, 0, 0, 0, 0, 1}, 0, {0x0231, 0x0221,      0, 0x7800}},	// 1!
	{{3, 0, 0, 0, 0, 1}, 0, {0x0332, 0x0340, 0x0300, 0x7900}},	// 2@
	{{3, 0, 0, 0, 0, 1}, 0, {0x0433, 0x0423,      0, 0x7A00}},	// 3#
	{{3, 0, 0, 0, 0, 1}, 0, {0x0534, 0x0534,      0, 0x7B00}},	// 4$
	{{3, 0, 0, 0, 0, 1}, 0, {0x0635, 0x0635,      0, 0x7C00}},	// 5%
	{{3, 0, 0, 0, 0, 1}, 0, {0x0736, 0x075E, 0x071E, 0x7D00}},	// 6^
	{{3, 0, 0, 0, 0, 1}, 0, {0x0837, 0x0826,      0, 0x7E00}},	// 7&
	{{3, 0, 0, 0, 0, 1}, 0, {0x0938, 0x092A,      0, 0x7F00}},	// 8*
	{{3, 0, 0, 0, 0, 1}, 0, {0x0A39, 0x0A28,      0, 0x8000}},	// 9(
	{{3, 0, 0, 0, 0, 1}, 0, {0x0B30, 0x0B29,      0, 0x8100}},	// 0)
	{{3, 0, 0, 0, 0, 1}, 0, {0x0C2D, 0x0C5F, 0x0C1F, 0x8200}},	// -_
	{{3, 0, 0, 0, 0, 1}, 0, {0x0D3D, 0x0D2B,      0, 0x8300}},	// =+
};

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
