#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#if CONFIG_SERIAL_PORT == 1
#define SERIAL_PORT	0x3F8
#elif CONFIG_SERIAL_PORT == 2
#define SERIAL_PORT	0x2F8
#elif CONFIG_SERIAL_PORT == 3
#define SERIAL_PORT	0x3E8
#elif CONFIG_SERIAL_PORT == 4
#define SERIAL_PORT	0x2E8
#endif

extern void preinit_console();

#endif	/* __CONSOLE_H__ */
