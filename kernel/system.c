#include <types.h>
#include <stddef.h>
#include <time.h>
#include "util.h"
#include "task.h"
#include "syscall.h"
#include "system.h"

void idle_task();
void sys_task();

task_func_t images[] = {
	idle_task,
	sys_task,
};

uint32_t _nr_boot_tasks = sizeof(images)/sizeof(images[0]);

void idle_task()
{
	while (TRUE) {
		uint32_t count;
	
		count = 20000;
		
		kprintf("idle.");

		/* Wait for a little while */
		while (count--);
	}
}

void sys_task()
{
	/* Our system task will run in user mode */
	switch_to_user_mode();
	
	while (TRUE) {
		uint32_t count;
		
		count = 20000;
		syscall_putstr("sys.");
		
		/* Wait for a little while */
		while (count--);
	}
}
