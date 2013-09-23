#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "debug.h"
#include "proc/process.h"

uint32_t _debug_level = DL_DBG;

const char *_dbglevel_string[] = {
	"NIL",
	"DBG",
	"INF",
	"WRN",
	"ERR",
};

const char *dbglevel_string(uint32_t level)
{
	if (level > DL_MAX) {
		level = 0;
	}
	
	return _dbglevel_string[level];
}

void panic(const char *file, uint32_t line, const char *msg)
{
	local_irq_disable();	// Disable all interrupts
	kprintf("KERNEL PANIC(%s) at %s:%d", msg, file, line);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	local_irq_disable();
	kprintf("ASSERTION FAILED(%s) at %s:%d\n", desc, file, line);
	for (; ; ) ;
}

