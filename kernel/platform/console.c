#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "hal/hal.h"
#include "hal/spinlock.h"
#include "kd.h"
#include "console.h"

struct klog_buffer {
	uint32_t level;
	u_char ch;
};
static struct klog_buffer _klog_buffer[10];

/* Lock for the kernel console */
static struct spinlock _console_lock;
/* Debug console operations */
struct console_output_ops *_debug_console_ops = NULL;
/* List of kernel console input operations */
struct list _console_input_ops = {
	.prev = &_console_input_ops,
	.next = &_console_input_ops
};

static struct ansi_parser _serial_ansi_parser;

uint16_t ansi_parser_filter(struct ansi_parser *parser, u_char ch)
{
	uint16_t ret = 0;

	if (parser->length < 0) {
		if (ch == 0x1B) {
			parser->length = 0;
			return 0;
		} else {
			return (uint16_t)ch;
		}
	} else {
		parser->buffer[parser->length++] = ch;

		/* Check for known sequence */
		if (parser->length == 2) {
			if (strncmp(parser->buffer, "[A", 2) == 0) {
				ret = CONSOLE_KEY_UP;
			} else if (strncmp(parser->buffer, "[B", 2) == 0) {
				ret = CONSOLE_KEY_DOWN;
			} else if (strncmp(parser->buffer, "[D", 2) == 0) {
				ret = CONSOLE_KEY_LEFT;
			} else if (strncmp(parser->buffer, "[C", 2) == 0) {
				ret = CONSOLE_KEY_RIGHT;
			} else if (strncmp(parser->buffer, "[H", 2) == 0) {
				ret = CONSOLE_KEY_HOME;
			} else if (strncmp(parser->buffer, "[F", 2) == 0) {
				ret = CONSOLE_KEY_END;
			}
		} else if (parser->length == 3) {
			if (strncmp(parser->buffer, "[3~", 3) == 0) {
				ret = CONSOLE_KEY_DEL;
			} else if (strncmp(parser->buffer, "[5~", 3) == 0) {
				ret = CONSOLE_KEY_PGUP;
			} else if (strncmp(parser->buffer, "[6~", 3) == 0) {
				ret = CONSOLE_KEY_PGDN;
			}
		}

		if (ret != 0 || parser->length == ANSI_PARSER_BUF_SIZE) {
			parser->length = -1;
		}
		return ret;
	}
}

void init_ansi_parser(struct ansi_parser *parser)
{
	parser->length = -1;
}

void serial_console_putc(char ch)
{
	if (ch == '\n') {
		serial_console_putc('\r');
	}

	outportb(SERIAL_PORT, ch);
	while (!(inportb(SERIAL_PORT + 5) & 0x20));
}

uint16_t serial_console_getc()
{
	u_char ch = inportb(SERIAL_PORT + 6);
	uint16_t converted;

	if ((ch & ((1 << 4) | (1 << 5))) && ch != 0xFF) {
		if (inportb(SERIAL_PORT + 5) & 0x01) {
			ch = inportb(SERIAL_PORT);

			/* Convert CR to NL, and DEL to Backspace */
			if (ch == '\r') {
				ch = '\n';
			} else if (ch == 0x7F) {
				ch = '\b';
			}

			/* Handle escape sequence */
			converted = ansi_parser_filter(&_serial_ansi_parser, ch);
			if (converted) {
				return converted;
			}
		}
	}

	return 0;
}

/* Serial port console input/output operation */
static struct console_output_ops _serial_console_output_ops = {
	.putc = serial_console_putc
};
static struct console_input_ops _serial_console_input_ops = {
	.getc = serial_console_getc
};

void register_console_input_ops(struct console_input_ops *ops)
{
	LIST_INIT(&ops->link);

	spinlock_acquire(&_console_lock);
	list_add_tail(&ops->link, &_console_input_ops);
	spinlock_release(&_console_lock);
}

void unregister_console_input_ops(struct console_input_ops *ops)
{
	spinlock_acquire(&_console_lock);
	list_del(&ops->link);
	spinlock_release(&_console_lock);
}

/* Setup the debug console */
void platform_preinit_console()
{
	uint8_t status = inportb(SERIAL_PORT + 6);

	if ((status & ((1 << 4) | (1 << 5))) && status != 0xFF) {
		outportb(SERIAL_PORT + 1, 0x00);	// Disable all interrupts
		outportb(SERIAL_PORT + 3, 0x80);	// Enable DLAB (set baud rate divisor)
		outportb(SERIAL_PORT + 0, 0x03);	// Set divisor to 3 (lo byte) 38400 baud
		outportb(SERIAL_PORT + 1, 0x00);	//		    (hi byte)
		outportb(SERIAL_PORT + 3, 0x03);	// 8 bits, no parity, one stop bit
		outportb(SERIAL_PORT + 2, 0xC7);	// Enable FIFO, clear them, with 14-byte threshold
		outportb(SERIAL_PORT + 4, 0x0B);	// Enable interrupts, RTS/DSR set

		/* Wait for transmit to be empty */
		while (!(inportb(SERIAL_PORT + 5) & 0x20));

		/* Register serial console input and output operation */
		init_ansi_parser(&_serial_ansi_parser);
		_debug_console_ops = &_serial_console_output_ops;
		register_console_input_ops(&_serial_console_input_ops);
	}
}

/* Setup the console */
void platform_init_console()
{
	;
}

static int kd_cmd_log(int argc, char **argv, kd_filter_t *filter)
{
	return 0;
}

/* Initialize the debug console */
void preinit_console()
{
	spinlock_init(&_console_lock, "console-lock");
	
	platform_preinit_console();

	/* Register the KD command */
	kd_register_cmd("log", "Display the kernel log buffer.", kd_cmd_log);
}
