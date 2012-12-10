#ifndef __TERMINAL_H__
#define __TERMINAL_H__

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

struct terminal_output_ops {
	/* Write a character to the terminal */
	void (*putc)(char ch);
};
typedef struct terminal_output_ops terminal_output_ops_t;

struct terminal_input_ops {
	/* Link to input operations list */
	struct list link;

	/* Get a character from terminal */
	uint16_t (*getc)();
};
typedef struct terminal_input_ops terminal_input_ops_t;

/* Special terminal key definitions */
#define TERMINAL_KEY_UP		0x100
#define TERMINAL_KEY_DOWN	0x101
#define TERMINAL_KEY_LEFT	0x102
#define TERMINAL_KEY_RIGHT	0x103
#define TERMINAL_KEY_HOME	0x104
#define TERMINAL_KEY_END	0x105
#define TERMINAL_KEY_PGUP	0x106
#define TERMINAL_KEY_PGDN	0x107
#define TERMINAL_KEY_DEL	0x108

#define ANSI_PARSER_BUF_SIZE	3

/* ANSI escape code parser structure */
struct ansi_parser {
	/* Buffer containing collected sequence */
	char buffer[ANSI_PARSER_BUF_SIZE];
	int length;	// Buffer length
};
typedef struct ansi_parser ansi_parser_t;

/* Serial debug terminal */
extern struct terminal_output_ops *_debug_terminal_ops;

/* Terminal input operation list */
extern struct list _terminal_input_ops;

extern void preinit_terminal();
extern void init_terminal();

#endif	/* __TERMINAL_H__ */
