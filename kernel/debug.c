#include <types.h>
#include "hal.h"
#include "matrix/debug.h"

uint32_t _debug_level = DL_DBG;

void panic(const char *message, const char *file, uint32_t line)
{
	disable_interrupt();	// Disable all interrupts
	kprintf("PANIC(%s) at %s:%d\n", message, file, line);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	disable_interrupt();
	kprintf("ASSERTION FAILED(%s) at %s:%d\n", desc, file, line);
	for (; ; ) ;
}

