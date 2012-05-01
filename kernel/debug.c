#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include "hal/hal.h"
#include "hal/lirq.h"
#include "matrix/debug.h"

uint32_t _debug_level = DL_DBG;

void panic(const char *file, uint32_t line, const char *message)
{
	irq_disable();
	kprintf("PANIC[%s:%d] %s\n", file, line, message);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	irq_disable();
	kprintf("ASSERTION FAILED[%s:%d] %s\n", file, line, desc);
	for (; ; ) ;
}
