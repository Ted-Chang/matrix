#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "list.h"

#if CONFIG_SERIAL_PORT == 1
#define SERIAL_PORT	0x3F8
#elif CONFIG_SERIAL_PORT == 2
#define SERIAL_PORT	0x2F8
#elif CONFIG_SERIAL_PORT == 3
#define SERIAL_PORT	0x3E8
#elif CONFIG_SERIAL_PORT == 4
#define SERIAL_PORT	0x2E8
#endif	/* CONFIG_SERIAL_PORT */

struct console_output_ops {
	/* Write a character to the console */
	void (*putc)(char ch);
};
typedef struct console_output_ops console_output_ops_t;

struct console_input_ops {
	/* Link to input operations list */
	struct list link;

	/* Get a character from console */
	uint16_t (*getc)();
};
typedef struct console_input_ops console_input_ops_t;

/* Special console key definitions */
#define CONSOLE_KEY_UP		0x100
#define CONSOLE_KEY_DOWN	0x101
#define CONSOLE_KEY_LEFT	0x102
#define CONSOLE_KEY_RIGHT	0x103
#define CONSOLE_KEY_HOME	0x104
#define CONSOLE_KEY_END		0x105
#define CONSOLE_KEY_PGUP	0x106
#define CONSOLE_KEY_PGDN	0x107
#define CONSOLE_KEY_DEL		0x108

#define ANSI_PARSER_BUF_SIZE	3

/* ANSI escape code parser structure */
struct ansi_parser {
	/* Buffer containing collected sequence */
	char buffer[ANSI_PARSER_BUF_SIZE];
	int length;	// Buffer length
};
typedef struct ansi_parser ansi_parser_t;

/* Serial debug console */
extern struct console_output_ops *_debug_console_ops;

extern void preinit_console();

#endif	/* __CONSOLE_H__ */
