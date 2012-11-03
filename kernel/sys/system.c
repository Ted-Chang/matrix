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
	char *init_argv[] = {
		"/init",
		NULL
	};

	DEBUG(DL_DBG, ("sys_init_proc: CURR_PROC(%p).\n", CURR_PROC));
	
	/* Run init process from executable file init */
	exec(init_argv[0], 1, init_argv);
	PANIC("sys_init_proc: should not get here");
}
