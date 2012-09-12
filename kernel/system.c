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
	sys_task,
};

uint32_t _nr_boot_tasks = sizeof(images)/sizeof(images[0]);

void idle_task(void *ctx)
{
	while (TRUE) {

		uint32_t count;
		char *buf;
		size_t buf_size;
	
		count = 20000;

		buf_size = 100;
		buf = kmalloc(buf_size, 0);
		if (!buf) {
			DEBUG(DL_ERR, ("kmalloc failed, buf_size(%d)\n", buf_size));
		} else {
			kfree(buf);
			buf_size++;
		}
		
		/* Wait for a little while */
		while (count--);
	}
}

void sys_task(void *ctx)
{
	/* Our system task will run in user mode */
	switch_to_user_mode();
	
	while (TRUE) {
		int rc = 0;
		int fd = 0;
		uint32_t count;
		char hostname[128];
		pid_t pid;
		uid_t uid;
		gid_t gid;
		
		count = 200000;

		fd = syscall_open("/test1.txt", 0, 0);
		if (fd == -1) {
			syscall_putstr("sys_task: open /test1.txt failed.\n");
		} else {
			int bytes = 0;
			char buf[1024] = {0};
			
			bytes = syscall_read(fd, buf, 1024);
			if (bytes != -1) {
				syscall_putstr(buf);
			} else {
				syscall_putstr("sys_task: read /test1.txt failed.\n");
			}
			
			syscall_close(fd);
		}

		fd = syscall_open("/", 0, 0);
		if (fd == -1) {
			syscall_putstr("sys_task: open root failed.\n");
		} else {
			int index;
			struct dirent d;

			index = 0;
			while (rc != -1) {
				memset(&d, 0, sizeof(struct dirent));
				rc = syscall_readdir(fd, index, &d);
				index++;
				if (rc != -1) {
					syscall_putstr(d.name);
					syscall_putstr("\n");
				} else {
					syscall_putstr("sys_task: no entry.\n");
				}
			}

			syscall_close(fd);
		}

		rc = syscall_gethostname(hostname, 128);
		if (rc == -1) {
			syscall_putstr("sys_task: gethostname failed.\n");
		} else {
			syscall_putstr(hostname);
			syscall_putstr("\n");
		}

		rc = syscall_sethostname("MATRIX", strlen("MATRIX"));
		if (rc == -1) {
			syscall_putstr("sys_task: sethostname failed.\n");
		}

		pid = syscall_getpid();

		uid = syscall_getuid();
		if (uid == 500) {
			syscall_putstr("sys_task: root uid.\n");
		}

		gid = syscall_getgid();
		if (gid == 500) {
			syscall_putstr("sys_task: root gid.\n");
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
		int fd = 0, root_fd = 0;
		uint32_t count;
		struct timeval tv;
		char buf[256] = {0};

		count = 20000;

		memset(&tv, 0, sizeof(struct timeval));
		rc = gettimeofday(&tv, NULL);
		if (rc != 0) {
			syscall_putstr("gettimeofday failed.\n");
		}

		/* Wait for a little while */
		while (count--);
	}
}
