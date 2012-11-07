#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "sys/time.h"
#include "matrix/debug.h"
#include "proc/process.h"
#include "mm/malloc.h"
#include "system.h"

void sys_init_thread()
{
	int rc = -1;
	const char *init_argv[] = {
		"/init",
		NULL
	};

	DEBUG(DL_DBG, ("sys_init_proc: CURR_PROC(%p).\n", CURR_PROC));

	while (TRUE) {
		int count = 0;
		kprintf("sys_init_thread: message.\n");
		while (count++ < 200000);
		count = 0;
	}
	
	/* Run init process from executable file init */
	rc = process_create(init_argv, _kernel_proc, 0, 16, NULL);
	if (rc != 0) {
		PANIC("sys_init_proc: could not start init process.\n");
	}
}

void sys_test_thread()
{
	while (TRUE) {
		int count = 0;
		kprintf("sys_test_thread: message.\n");
		while (count++ < 200000);
		count = 0;
	}
}
