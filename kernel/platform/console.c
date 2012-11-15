#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "kd.h"
#include "console.h"

void serial_console_putc(char ch)
{
	if (ch == '\n') {
		serial_console_putc('\r');
	}

	outportb(SERIAL_PORT, ch);
	while (!(inportb(SERIAL_PORT + 5) & 0x20));
}

uint16_t ansi_parser_filter(u_char ch)
{
	uint16_t ret = 0;

	return ret;
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
			converted = ansi_parser_filter(ch);
			if (converted) {
				return converted;
			}
		}
	}

	return 0;
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
	platform_preinit_console();

	/* Register the KD command */
	kd_register_cmd("log", "Display the kernel log buffer.", kd_cmd_log);
}
