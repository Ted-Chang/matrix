#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"
#include "proc/process.h"
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

void init_task(void *ctx)
{
	while (TRUE) {
		;
	}
}

void idle_task(void *ctx)
{
	while (TRUE) {

		int rc = -1;
		int fd;
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

		fd = open("/", 0, 0);
		if (fd == -1) {
			DEBUG(DL_ERR, ("open root failed.\n"));
		} else {
			int index = 0;
			struct dirent d;

			while (rc != -1) {
				memset(&d, 0, sizeof(struct dirent));
				rc = readdir(fd, index, &d);
				index++;
				if (rc != -1) {
					kprintf("%s\n", d.name);
				} else {
					kprintf("no entry\n");
				}
			}

			close(fd);
		}
		
		/* Wait for a little while */
		timer_delay(1000);
	}
}
