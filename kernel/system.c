#include <types.h>
#include <stddef.h>
#include <time.h>
#include "util.h"
#include "proc/task.h"
#include "syscall.h"
#include "system.h"

void idle_task();
void sys_task();

task_func_t images[] = {
	idle_task,
	sys_task,
};

uint32_t _nr_boot_tasks = sizeof(images)/sizeof(images[0]);

void idle_task(void *ctx)
{
	while (TRUE) {
		uint32_t count;
	
		count = 20000;
		
		kprintf("idle.");

		/* Wait for a little while */
		while (count--);
	}
}

void sys_task(void *ctx)
{
	/* Our system task will run in user mode */
	switch_to_user_mode();
	
	while (TRUE) {
		int fd = 0;
		uint32_t count;
		
		count = 20000;
		syscall_putstr("sys.");

		fd = syscall_open("/etc/sys", 0, 0);
		if (fd == -1) {
			syscall_putstr("open sys-file failed.\n");
			continue;
		}
		
		/* Wait for a little while */
		while (count--);
	}
}

void init_task(void *ctx)
{
	switch_to_user_mode();

	while (TRUE) {
		int fd = 0;
		uint32_t count;

		count = 20000;
		syscall_putstr("init.");

		fd = syscall_open("/etc/init", 0, 0);
		if (fd == -1) {
			syscall_putstr("open init-file failed.\n");
			continue;
		}

		/* Wait for a little while */
		while (count--);
	}
}
