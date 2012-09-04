#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"
#include "proc/process.h"
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
		
		kprintf("idle.\n");

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
		syscall_putstr("sys.\n");

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
	int rc = 0;
	
	switch_to_user_mode();

	while (TRUE) {
		int fd = 0;
		uint32_t count;
		struct timeval tv;
		char buf[256] = {0};

		count = 20000;
		syscall_putstr("init.\n");

		memset(&tv, 0, sizeof(struct timeval));
		rc = gettimeofday(&tv, NULL);
		if (rc != 0) {
			syscall_putstr("gettimeofday failed.\n");
		} else {
			//vsprintf(buf, "timeval.tv_sec(%d\n), timeval.tv_usec(%d)\n",
			//	 tv.tv_sec, tv.tv_usec);
			syscall_putstr("gettimeofday successfully.\n");
		}

		fd = syscall_open("/etc/init", 0, 0);
		if (fd == -1) {
			syscall_putstr("open init-file failed.\n");
			continue;
		}

		/* Wait for a little while */
		while (count--);
	}
}
