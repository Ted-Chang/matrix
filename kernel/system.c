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

void sys_task(void *ctx)
{
	/* Our system task will run in user mode */
	//switch_to_user_mode();
	
	while (TRUE) {
		/* int rc = 0; */
		/* int fd = 0; */
		/* char hostname[128]; */
		/* pid_t pid; */
		/* uid_t uid; */
		/* gid_t gid; */
		
		/* fd = mtx_open("/test1.txt", 0, 0); */
		/* if (fd == -1) { */
		/* 	mtx_putstr("sys_task: open /test1.txt failed.\n"); */
		/* } else { */
		/* 	int bytes = 0; */
		/* 	char buf[1024] = {0}; */
			
		/* 	bytes = mtx_read(fd, buf, 1024); */
		/* 	if (bytes != -1) { */
		/* 		mtx_putstr(buf); */
		/* 	} else { */
		/* 		mtx_putstr("sys_task: read /test1.txt failed.\n"); */
		/* 	} */
			
		/* 	mtx_close(fd); */
		/* } */

		/* fd = mtx_open("/", 0, 0); */
		/* if (fd == -1) { */
		/* 	mtx_putstr("sys_task: open root failed.\n"); */
		/* } else { */
		/* 	int index; */
		/* 	struct dirent d; */

		/* 	index = 0; */
		/* 	while (rc != -1) { */
		/* 		memset(&d, 0, sizeof(struct dirent)); */
		/* 		rc = mtx_readdir(fd, index, &d); */
		/* 		index++; */
		/* 		if (rc != -1) { */
		/* 			mtx_putstr(d.name); */
		/* 			mtx_putstr("\n"); */
		/* 		} else { */
		/* 			mtx_putstr("sys_task: no entry.\n"); */
		/* 		} */
		/* 	} */

		/* 	mtx_close(fd); */
		/* } */

		/* rc = mtx_gethostname(hostname, 128); */
		/* if (rc == -1) { */
		/* 	mtx_putstr("sys_task: gethostname failed.\n"); */
		/* } else { */
		/* 	mtx_putstr(hostname); */
		/* 	mtx_putstr("\n"); */
		/* } */

		/* rc = mtx_sethostname("MATRIX", strlen("MATRIX")); */
		/* if (rc == -1) { */
		/* 	mtx_putstr("sys_task: sethostname failed.\n"); */
		/* } */

		/* pid = mtx_getpid(); */

		/* uid = mtx_getuid(); */
		/* if (uid == 500) { */
		/* 	mtx_putstr("sys_task: root uid.\n"); */
		/* } */

		/* gid = mtx_getgid(); */
		/* if (gid == 500) { */
		/* 	mtx_putstr("sys_task: root gid.\n"); */
		/* } */
		
		/* /\* Wait for a little while *\/ */
		/* mtx_sleep(1000); */
	}
}

void init_task(void *ctx)
{
	int rc = 0;
	
	//switch_to_user_mode();

	while (TRUE) {
		/* int fd = 0, root_fd = 0; */
		/* struct timeval tv; */
		/* char buf[256] = {0}; */

		/* memset(&tv, 0, sizeof(struct timeval)); */
		/* rc = gettimeofday(&tv, NULL); */
		/* if (rc != 0) { */
		/* 	mtx_putstr("gettimeofday failed.\n"); */
		/* } */

		/* /\* Wait for a little while *\/ */
		/* mtx_sleep(1000); */
	}
}
