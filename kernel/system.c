#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"
#include "proc/process.h"
#include "syscall.h"
#include "system.h"
#include "matrix/debug.h"
#include "fs.h"
#include "mm/malloc.h"

void idle_task();
void sys_task();

task_func_t images[] = {
	idle_task,
};

uint32_t _nr_boot_tasks = sizeof(images)/sizeof(images[0]);

void idle_task(void *ctx)
{
	while (TRUE) {

		char *buf;
		size_t buf_size;
	
		buf_size = 100;
		buf = kmalloc(buf_size, 0);
		if (!buf) {
			DEBUG(DL_ERR, ("kmalloc failed, buf_size(%d)\n", buf_size));
		} else {
			kfree(buf);
			buf_size++;
		}
		
		/* Wait for a little while */
		timer_delay(1000);
	}
}
