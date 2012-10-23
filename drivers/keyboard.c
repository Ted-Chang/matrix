#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "hal/isr.h"
#include "matrix/debug.h"
#include "util.h"
#include "keyboard.h"

/* Keyboard I/O ports */
#define KBD_STATUS_PORT		0x64
#define KBD_DATA_PORT		0x60

#define KBD_CTRL_TIMEOUT	0x10000
/* Keyboard command */
#define KBD_CMD_SET_LEDS	0xED

/* Make code */
#define MK_CTRL			0x1D
#define MK_LSHIFT		0x2A
#define MK_RSHIFT		0x36
#define MK_ALT			0x38
#define MK_CAPS			0x3A
#define MK_NUMLK		0x45
#define MK_SCRLK		0x46
#define MK_SYSRQ		0x54
#define MK_INSERT		0x52

#define BREAK			0x0080

/* Flags - active */
#define ACT_SCRLK     0x0010
#define ACT_NUMLK     0x0020
#define ACT_CAPSLK    0x0040
#define ACT_INSERT    0x0080
/* Flag */
#define FLAG_RSHIFT   0x0001
#define FLAG_LSHIFT   0x0002
#define FLAG_CTRL     0x0004
#define FLAG_ALT      0x0008
#define FLAG_LCTRL    0x0100
#define FLAG_LALT     0x0200
#define FLAG_SYSRQ    0x0400
#define FLAG_SUSPEND  0x0800
#define FLAG_SCRLK    0x1000
#define FLAG_NUMLK    0x2000
#define FLAG_CAPS     0x4000
#define FLAG_INSTERT  0x8000
/* Mode */
#define MODE_E1       0x01
#define MODE_E0       0x02
#define MODE_RCTRL    0x04
#define MODE_RALT     0x08
#define MODE_EINST    0x10
#define MODE_FNUMLK   0x20
#define MODE_LASTID   0x40
#define MODE_READID   0x80
/* Led */
#define LEGS_MASK     0x07
#define LED_SCRLK     0x01
#define LED_NUMLK     0x02
#define LED_CAPSLK    0x04
#define LED_CIRCUS    0x08
#define LED_ACKRCV    0x10
#define LED_RSRCV     0x20
#define LED_MODEUPD   0x40
#define LED_TRANSERR  0x80


/* Key code definition */
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

/* Keyboard status */
union kbd_std_status {
	uint16_t data;
	struct {
		uint8_t rshift:1;
		uint8_t lshift:1;
		uint8_t ctrl:1;
		uint8_t alt:1;
		uint8_t scrlk_active:1;
		uint8_t numlk_active:1;
		uint8_t capslk_active:1;
		uint8_t reserved1:1;
		uint8_t lctrl:1;
		uint8_t lalt:1;
		uint8_t rctrl:1;
		uint8_t ralt:1;
		uint8_t e0_prefix:1;
		uint8_t reserved2:1;
		uint8_t shift:1;
		uint8_t reserved3:1;
	} status;
};

/* Keyboard control status */
union kbd_ctrl_status {
	uint8_t data;
	struct {
		uint8_t outbuf_full:1;
		uint8_t inbuf_full:1;
		uint8_t self_test:1;
		uint8_t command:1;
		uint8_t unlocked:1;
		uint8_t mouse_outbuf_full:1;
		uint8_t general_timeout:1;
		uint8_t parity_error:1;
	} status;
};

/* Keyboard global mode data */
struct kbd_data {
	uint16_t kbd_flag;
	uint16_t kbd_mode;
	uint16_t kbd_led;
};

struct kbd_cmd {
	uint8_t cmd;
	uint8_t data_len;
	uint8_t *data;
};

/* Keyboard IRQ hook */
struct irq_hook _kbd_hook;

static char _scan_code;
static union kbd_std_status _kbdst;

static uint16_t _ack_left;
static uint8_t _ack_cmd;
static uint8_t _led_state;
static struct kbd_data _kbd_data;

#define KBD_Q_MAX_SIZE	32
static uint8_t _kbd_buf[32];
static uint16_t _rawq_head;
static uint16_t _rawq_tail;
static uint16_t _kbd_bufhead;
static uint16_t _kbd_buftail;

const struct key_code _keymap[128] = {
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
	{{3, 0, 0, 0, 0, 1}, 0, {0x0E08, 0x0E08, 0x0E7F, 0x0E00}},	// backspace
	{{3, 0, 0, 0, 0, 1}, 0, {0x0F09, 0x0F00, 0x9400, 0xA500}},	// tab
	{{3, 0, 0, 0, 1, 1}, 0, {0x1071, 0x1051, 0x1011, 0x1000}},	// Q
	{{3, 0, 0, 0, 1, 1}, 0, {0x1177, 0x1157, 0x1117, 0x1100}},	// W
	{{3, 0, 0, 0, 1, 1}, 0, {0x1265, 0x1245, 0x1205, 0x1200}},	// E
	{{3, 0, 0, 0, 1, 1}, 0, {0x1372, 0x1352, 0x1312, 0x1300}},	// R
	{{3, 0, 0, 0, 1, 1}, 0, {0x1474, 0x1454, 0x1414, 0x1400}},	// T
	{{3, 0, 0, 0, 1, 1}, 0, {0x1579, 0x1559, 0x1519, 0x1500}},	// Y
	{{3, 0, 0, 0, 1, 1}, 0, {0x1675, 0x1655, 0x1615, 0x1600}},	// U
	{{3, 0, 0, 0, 1, 1}, 0, {0x1769, 0x1749, 0x1709, 0x1700}},	// I
	{{3, 0, 0, 0, 1, 1}, 0, {0x186F, 0x184F, 0x180F, 0x1800}},	// O
	{{3, 0, 0, 0, 1, 1}, 0, {0x1970, 0x1950, 0x1910, 0x1900}},	// P
	{{3, 0, 0, 0, 0, 1}, 0, {0x1A5B, 0x1A7B, 0x1A1B, 0x1A00}},	// [{
	{{3, 0, 0, 0, 0, 1}, 0, {0x1B5D, 0x1B7D, 0x1B1D, 0x1B00}},	// ]}
	{{3, 0, 0, 0, 0, 1}, 0, {0x1C0D, 0x1C0D, 0x1C0A, 0xA600}},	// Enter
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// L Ctrl
	{{3, 0, 0, 0, 1, 1}, 0, {0x1E61, 0x1E41, 0x1E01, 0x1E00}},	// A
	{{3, 0, 0, 0, 1, 1}, 0, {0x1F73, 0x1F53, 0x1F13, 0x1F00}},	// S
	{{3, 0, 0, 0, 1, 1}, 0, {0x2064, 0x2044, 0x2004, 0x2000}},	// D
	{{3, 0, 0, 0, 1, 1}, 0, {0x2166, 0x2146, 0x2106, 0x2100}},	// F
	{{3, 0, 0, 0, 1, 1}, 0, {0x2267, 0x2247, 0x2207, 0x2200}},	// G
	{{3, 0, 0, 0, 1, 1}, 0, {0x2368, 0x2348, 0x2308, 0x2300}},	// H
	{{3, 0, 0, 0, 1, 1}, 0, {0x246A, 0x244A, 0x240A, 0x2400}},	// J
	{{3, 0, 0, 0, 1, 1}, 0, {0x256B, 0x254B, 0x250B, 0x2500}},	// K
	{{3, 0, 0, 0, 1, 1}, 0, {0x266C, 0x264C, 0x260C, 0x2600}},	// L
	{{3, 0, 0, 0, 0, 1}, 0, {0x273B, 0x273A,      0, 0x2700}},	// ;:
	{{3, 0, 0, 0, 0, 1}, 0, {0x2827, 0x2822,      0,      0}},	// '"
	{{3, 0, 0, 0, 0, 1}, 0, {0x2960, 0x297E,      0,      0}},	// `~
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// L Shift
	{{3, 0, 0, 0, 0, 1}, 0, {0x2B5C, 0x2B7C, 0x2B1C, 0x2600}},	// \|
	{{3, 0, 0, 0, 1, 1}, 0, {0x2C7A, 0x2C5A, 0x2C1A, 0x2C00}},	// Z
	{{3, 0, 0, 0, 1, 1}, 0, {0x2D78, 0x2D58, 0x2D18, 0x2D00}},	// X
	{{3, 0, 0, 0, 1, 1}, 0, {0x2E63, 0x2E43, 0x2E03, 0x2E00}},	// C
	{{3, 0, 0, 0, 1, 1}, 0, {0x2F76, 0x2F56, 0x2F16, 0x2F00}},	// V
	{{3, 0, 0, 0, 1, 1}, 0, {0x3062, 0x3042, 0x3002, 0x3000}},	// B
	{{3, 0, 0, 0, 1, 1}, 0, {0x316E, 0x314E, 0x310E, 0x3100}},	// N
	{{3, 0, 0, 0, 1, 1}, 0, {0x326D, 0x324D, 0x320D, 0x3200}},	// M
	{{3, 0, 0, 0, 0, 1}, 0, {0x332C, 0x333C,      0,      0}},	// ,<
	{{3, 0, 0, 0, 0, 1}, 0, {0x342E, 0x343E,      0,      0}},	// .>
	{{3, 0, 0, 0, 0, 1}, 0, {0x352F, 0x353F,      0,      0}},	// /?
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// R Shift
	{{3, 0, 0, 1, 0, 1}, 0, {0x372A, 0x372A, 0x9600, 0x3700}},	// *
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// L Alt
	{{3, 0, 0, 0, 0, 1}, 0, {0x3920, 0x3920, 0x3920, 0x3920}},	// Space
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// Caps lock
	{{3, 0, 0, 0, 0, 1}, 0, {0x3B00, 0x5400, 0x5E00, 0x6800}},	// F1
	{{3, 0, 0, 0, 0, 1}, 0, {0x3C00, 0x5500, 0x5F00, 0x6900}},	// F2
	{{3, 0, 0, 0, 0, 1}, 0, {0x3D00, 0x5600, 0x6000, 0x6A00}},	// F3
	{{3, 0, 0, 0, 0, 1}, 0, {0x3E00, 0x5700, 0x6100, 0x6B00}},	// F4
	{{3, 0, 0, 0, 0, 1}, 0, {0x3F00, 0x5800, 0x6200, 0x6C00}},	// F5
	{{3, 0, 0, 0, 0, 1}, 0, {0x4000, 0x5900, 0x6300, 0x6D00}},	// F6
	{{3, 0, 0, 0, 0, 1}, 0, {0x4100, 0x5A00, 0x6400, 0x6E00}},	// F7
	{{3, 0, 0, 0, 0, 1}, 0, {0x4200, 0x5B00, 0x6500, 0x6F00}},	// F8
	{{3, 0, 0, 0, 0, 1}, 0, {0x4300, 0x5C00, 0x6600, 0x7000}},	// F9
	{{3, 0, 0, 0, 0, 1}, 0, {0x4400, 0x5D00, 0x6700, 0x7100}},	// F10
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// Num lock
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},	// Scroll lock
	{{3, 0, 0, 1, 0, 1}, 0, {0x4700, 0x4737, 0x7700, 0x9700}},	// 7 Home
	{{3, 0, 0, 1, 0, 1}, 0, {0x4800, 0x4838, 0x8D00, 0x9800}},	// 8 UP
	{{3, 0, 0, 1, 0, 1}, 0, {0x4900, 0x4939, 0x8400, 0x9900}},	// 0 PgUp
	{{3, 0, 0, 1, 0, 1}, 0, {0x4A2D, 0x4A2D, 0x8E00, 0x4A00}},	// -
	{{3, 0, 0, 1, 0, 1}, 0, {0x4B00, 0x4B34, 0x7300, 0x9B00}},	// 4 Left
	{{3, 0, 0, 1, 0, 1}, 0, {0x0000, 0x4C35, 0x8F00,      0}},	// 5
	{{3, 0, 0, 1, 0, 1}, 0, {0x4D00, 0x4D36, 0x7400, 0x9D00}},	// 6 Right
	{{3, 0, 0, 1, 0, 1}, 0, {0x4E2B, 0x4E2B,      0, 0x4E00}},	// +
	{{3, 0, 0, 1, 0, 1}, 0, {0x4F00, 0x4F31, 0x7500, 0x9F00}},	// 1 End
	{{3, 0, 0, 1, 0, 1}, 0, {0x5000, 0x5032, 0x9100, 0xA000}},	// 2 Down
	{{3, 0, 0, 1, 0, 1}, 0, {0x5100, 0x5133, 0x7600, 0xA100}},	// 3 PgDn
	{{3, 0, 0, 1, 0, 1}, 0, {0x5200, 0x5230, 0x9200, 0xA200}},	// 0 Ins
	{{3, 0, 0, 1, 0, 1}, 0, {0x5300, 0x532E, 0x9300, 0xA300}},	// Del
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},
	{{3, 0, 0, 0, 0, 1}, 0, {     0,      0,      0,      0}},
	{{3, 0, 0, 0, 0, 1}, 0, {0x8500, 0x8700, 0x8900, 0x8B00}},	// F11
	{{3, 0, 0, 0, 0, 1}, 0, {0x8600, 0x8800, 0x8A00, 0x8C00}},	// F12
};

static struct kbd_cmd _cmd_table[] = {
	{KBD_CMD_SET_LEDS, 1, &_led_state}	// After the ACK, led state must be sent
};
#define NR_KBD_CMDS	(sizeof(_cmd_table)/sizeof(struct kbd_cmd))

static void kbd_state_reset()
{
	_scan_code = 0;
	_kbdst.data = 0;
}

static void rawq_put(int8_t code)
{
	irq_disable();
	
	/* Put the key into the key queue and update the raw queue with
	 * new scan code.
	 */
	_kbd_buf[_rawq_tail] = code;
	_rawq_tail = (_rawq_tail + 2) % KBD_Q_MAX_SIZE;
	if (_rawq_tail == _rawq_head) {
		_rawq_head = (_rawq_head + 2) % KBD_Q_MAX_SIZE;
	}
	
	irq_enable();
}

static uint8_t rawq_read()
{
	uint16_t rawq_index;
	/* Read the raw queue incrementally, based on queue_index from head to tail */
	rawq_index = (_kbd_buftail + 1) % KBD_Q_MAX_SIZE;
	if (_rawq_tail == rawq_index) {
		return 0;	// No char in queue
	} else {
		uint8_t scancode = _kbd_buf[rawq_index];
		return scancode;
	}
}

static void keyq_clear()
{
	/* Clear raw queue and key queue */
	_rawq_head = _rawq_tail = 1;
	_kbd_bufhead = _kbd_buftail = 0;
}

static void keyq_put(uint16_t code)
{
	uint16_t keyq_index;
	uint16_t rawq_index;

	keyq_index = _kbd_buftail;
	rawq_index = (keyq_index + 1) % KBD_Q_MAX_SIZE;
	irq_disable();
	_kbd_buf[keyq_index] = (uint8_t)code;
	_kbd_buf[rawq_index] = (uint8_t)code >> 8;
	/* Update key queue buffer tail */
	_kbd_buftail = keyq_index;
	irq_enable();
}

static uint8_t keyq_get()
{
	if (_kbd_buftail == _kbd_bufhead) {
		_scan_code = 0;	// No char in queue
		return 0;
	} else {
		uint32_t i;
		i = _kbd_bufhead;
		irq_disable();
		_kbd_bufhead = (i + 2) % KBD_Q_MAX_SIZE;
		irq_enable();
		_scan_code = _kbd_buf[i+1];
		return _kbd_buf[i];
	}
}

static int wait_kbd(boolean_t op_read)
{
	int i;
	int8_t t;

	for (i = 0; i < KBD_CTRL_TIMEOUT; i++) {
		if (op_read)
			;
		else
			t = inportb(KBD_DATA_PORT);
	}
	
	if (i < KBD_CTRL_TIMEOUT)
		return 0;
	else
		return -1;
}

static int kbd_layout_decode(uint8_t c, union kbd_std_status st)
{
	int ret = -1;

	return ret;
}

static int kbd_wait_read()
{
	uint32_t i;
	uint8_t t;
	union kbd_ctrl_status st;
	
	for (i = 0, st.data = inportb(KBD_STATUS_PORT);
	     st.status.outbuf_full && i < KBD_CTRL_TIMEOUT;
	     i++, st.data = inportb(KBD_STATUS_PORT)) {
		t = inportb(KBD_DATA_PORT);
	}

	if (i < KBD_CTRL_TIMEOUT)
		return 0;

	return -1;
}

static int kbd_wait_write()
{
	uint32_t i;
	union kbd_ctrl_status st;

	for (i = 0, st.data = inportb(KBD_STATUS_PORT);
	     st.status.inbuf_full && i < KBD_CTRL_TIMEOUT;
	     i++, st.data = inportb(KBD_STATUS_PORT)) {
		timer_delay(1);
	}

	if (i < KBD_CTRL_TIMEOUT)
		return 0;
	
	return -1;
}

static int kbd_write(uint8_t data)
{
	if (kbd_wait_write() != -1) {
		outportb(KBD_DATA_PORT, data);
		return 0;
	} else {
		return -1;
	}
}

static int kbd_send_cmd(uint8_t cmd)
{
	if (_ack_left == 0) {
		if (kbd_write(cmd) != -1) {
			_ack_left = 2;	// Always need 2 ACKs
			return 0;
		}
	}

	DEBUG(DL_ERR, ("kbd_send_cmd: sending control CMD error\n"));
	return -1;
}

static void kbd_ack_hdlr()
{
	uint32_t i, j;

	switch (_ack_left) {
	case 2:
		for (i = 0; i < NR_KBD_CMDS; i++) {
			if (_ack_cmd == _cmd_table[i].cmd) {
				for (j = 0; j < _cmd_table[i].data_len; j++)
					kbd_write(_cmd_table[i].data[j]);
			}
		}
		_ack_left--;
		break;
	case 1:
		_ack_left--;
		break;
	default:
		DEBUG(DL_ERR, ("kbd_ack_hdlr: CRITICAL_ERROR\n"));
		break;
	}
}

static uint16_t kbd_decode(uint8_t c, union kbd_std_status st)
{
	int res = 0;

	res = kbd_layout_decode(c, st);
	if (res < 0) {

		/* Set the keymap index value according to the shift flags */
		if (st.status.alt) {
			res = 3;
		} else if (st.status.ctrl) {
			res = 2;
		} else if (st.status.e0_prefix && _keymap[c].flags.numlock_af) {
			res = 0;
		} else {
			/* Letter keys have to be handled according to CAPSLOCK
			 * Keypad keys have to be handled according to NUMLOCK
			 */
			int caps_or_num = (_keymap[c].flags.numlock_af && _kbdst.status.numlk_active) ||
				(_keymap[c].flags.capslock_af && _kbdst.status.capslk_active);
			if (_kbdst.status.shift)
				res = (1 ^ caps_or_num);
			else	// Extended key shouldn't shift
				res = (0 ^ caps_or_num);
		}

		res = _keymap[c].data[res];
	}

	return res;
}

static int preprocess(u_char code)
{
	int rc = -1;
	int e0_keycode_1 = 0;
	static int e0_keycode = 0;	// It's a static variable

	/* Add the extended key prefix */
	if (e0_keycode) {
		/* Extended keycode */
		e0_keycode_1 = 0x80;
		e0_keycode = 0;
	}
	
	switch (code) {
	case 0xE0:
		e0_keycode = 1;
		break;
	case 0xFA:
		kbd_ack_hdlr();
		break;
	case MK_INSERT:
		_kbd_data.kbd_flag ^= ACT_INSERT;
		rc = 0;
		break;
	case MK_CTRL:
		_kbdst.status.ctrl = 1;
		_kbd_data.kbd_flag |= FLAG_CTRL;
		if (!e0_keycode_1) {
			_kbdst.status.lctrl = 1;
			_kbd_data.kbd_flag |= FLAG_LCTRL;
		} else {	// Extended RCTRL
			_kbdst.status.rctrl = 1;
			_kbd_data.kbd_mode |= MODE_RCTRL;
		}
		break;
	case MK_LSHIFT:
		_kbdst.status.lshift = _kbdst.status.rshift = 1;
		_kbd_data.kbd_flag |= FLAG_LSHIFT;
		/* Numlock and shift conditions automatically cared in extended key */
		break;
	case MK_RSHIFT:
		_kbdst.status.lshift = _kbdst.status.rshift = 1;
		_kbd_data.kbd_flag |= FLAG_RSHIFT;
		break;
	case MK_ALT:
		_kbdst.status.alt = 1;
		_kbd_data.kbd_flag |= FLAG_ALT;
		if (!e0_keycode_1) {
			_kbdst.status.lalt = 1;
			_kbd_data.kbd_flag |= FLAG_LALT;
		} else {
			_kbdst.status.ralt = 1;
			_kbd_data.kbd_mode |= MODE_RALT;
		}
		break;
	case MK_CAPS:
		_kbdst.status.capslk_active ^= 1;
		_kbd_data.kbd_flag |= FLAG_CAPS;
		_kbd_data.kbd_flag ^= ACT_CAPSLK;
		_kbd_data.kbd_led ^= LED_CAPSLK;
		_led_state = _kbd_data.kbd_led & LEGS_MASK;
		kbd_send_cmd(KBD_CMD_SET_LEDS);
		break;
	case MK_NUMLK:
		_kbdst.status.numlk_active ^= 1;
		_kbd_data.kbd_flag |= FLAG_NUMLK;
		_kbd_data.kbd_flag ^= ACT_NUMLK;
		_kbd_data.kbd_led ^= LED_NUMLK;
		_led_state = _kbd_data.kbd_led & LEGS_MASK;
		kbd_send_cmd(KBD_CMD_SET_LEDS);
		break;
	case MK_SCRLK:
		_kbdst.status.scrlk_active ^= 1;
		_kbd_data.kbd_flag |= FLAG_SCRLK;
		_kbd_data.kbd_flag ^= ACT_SCRLK;
		_kbd_data.kbd_led ^= LED_SCRLK;
		_led_state = _kbd_data.kbd_led & LEGS_MASK;
		kbd_send_cmd(KBD_CMD_SET_LEDS);
		break;
	case MK_SYSRQ:
		_kbd_data.kbd_flag |= FLAG_SYSRQ;
		DEBUG(DL_DBG, ("preprocess: SYSRQ pressed...\n"));
		break;
		/* Normal function key released */
	case BREAK|MK_CTRL:
		break;
	case BREAK|MK_LSHIFT:
		break;
	case BREAK|MK_RSHIFT:
		break;
	case BREAK|MK_ALT:
		break;
	case BREAK|MK_NUMLK:
		break;
	case BREAK|MK_SCRLK:
		break;
	case BREAK|MK_SYSRQ:
		break;
	default:
		if (!(BREAK & code))
			rc = 0;
		break;
	}

	if (!rc)
		rawq_put(code | e0_keycode_1);

	return rc;
}

void postprocess()
{
	uint16_t decoded = 0;
	uint8_t code = rawq_read();

	while (code != 0) {

		if (code & 0x80) {	// Extended code
			_kbdst.status.e0_prefix = 1;
			code &= 0x7F;
		}

		/* Handle the basic key */
		decoded = kbd_decode(code, _kbdst);

		/* Reset the e0_prefix flag */
		_kbdst.status.e0_prefix = 0;

		if (decoded == 0) {
			/* Skip the key stroke */
			keyq_put(0x0000);
			keyq_get();
		} else {
			keyq_put(decoded);
			decoded = 0;
		}

		code = rawq_read();
	}
}

static void kbd_callback(struct registers *regs)
{
	u_char scan_code;

	/* We are the interrupt handler. If we are called, we assume that there is
	 * something to be read. (so we don't read the status port.)
	 */
	scan_code = inportb(KBD_DATA_PORT);

	/* We don't check the error code here */
	if (!preprocess(scan_code)) {
		irq_enable();	// Reenable interrupts
		postprocess();	// Do post process
	}
	
	irq_done(regs->int_no);
}

void init_keyboard()
{
	/* Choose keyboard layout */
	//...
	
	/* Reset keyboard status */
	kbd_state_reset();

	register_irq_handler(IRQ1, &_kbd_hook, kbd_callback);

	/* Clear the keyboard queue */
	keyq_clear();
}
