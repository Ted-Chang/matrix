#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "sys/time.h"
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "list.h"
#include "hal/cpu.h"
#include "proc/process.h"
#include "mm/malloc.h"
#include "system.h"
#include "keyboard.h"
#include "floppy.h"

void load_modules()
{
	init_keyboard();
	kprintf("Keyboard driver initialization done.\n");

	init_floppy();
	kprintf("Floppy driver initialization done.\n");
}

void sys_init_thread(void *ctx)
{
	int rc = -1;
	struct list *l;
	struct cpu *c;
	const char *init_argv[] = {
		"/init",
		"-d",
		NULL
	};

	/* Dump all the CPU in the system */
	LIST_FOR_EACH(l, &_running_cpus) {
		c = LIST_ENTRY(l, struct cpu, link);
		dump_cpu(c);
	}

	/* Load the modules */
	load_modules();

	/* Run init process from executable file init */
	rc = process_create(init_argv, _kernel_proc, 0, 16, NULL);
	if (rc != 0) {
		PANIC("sys_init_proc: could not start init process.\n");
	}
}
