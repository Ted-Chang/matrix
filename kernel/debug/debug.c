#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "matrix/debug.h"
#include "proc/process.h"

uint32_t _debug_level = DL_DBG;

void panic(const char *message, const char *file, uint32_t line)
{
	irq_disable();	// Disable all interrupts
	kprintf("PANIC(%s) at %s:%d\n", message, file, line);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	irq_disable();
	kprintf("ASSERTION FAILED(%s) at %s:%d\n", desc, file, line);
	for (; ; ) ;
}

