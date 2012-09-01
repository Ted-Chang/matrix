/*
 * syscall.c
 */
#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include "hal.h"
#include "syscall.h"
#include "isr.h"	// register_interrupt_handler
#include "util.h"	// putstr
#include "proc/task.h"
#include "div64.h"	// do_div

static void syscall_handler(struct registers *regs);

static struct irq_hook _syscall_hook;

int open(const char *file, int flags, int mode)
{
	int fd = -1;
	struct vfs_node *n;

	/* n = ; */
	/* if (!n && FLAG_ON(flags, 0x600)) { */
	/* 	; */
	/* } */
	
	/* if (!n) */
	/* 	return -1; */

	/* fd = ; */

	return fd;
}

int close(int fd)
{
	int rc = -1;

	if (fd >= CURRENT_PROC->fds->len || fd < 0)
		return -1;

	vfs_close(CURRENT_PROC->fds->nodes[fd]);
	
	return 0;
}

int read(int fd, char *buf, int len)
{
	uint32_t out = -1;

	if (fd >= CURRENT_PROC->fds->len || fd < 0)
		return -1;
	if (CURRENT_PROC->fds->nodes[fd] == NULL)
		return -1;

	return out;
}

int write(int fd, char *buf, int len)
{
	uint32_t out = -1;
	
	if (fd >= CURRENT_PROC->fds->len || fd < 0)
		return -1;
	if (CURRENT_PROC->fds->nodes[fd] == NULL)
		return -1;
	
	return out;
}

int exit(int rc)
{
	return rc;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	struct tm t;
	useconds_t usecs;

	get_cmostime(&t);
	tv->tv_sec = 0;
	tv->tv_usec = 0;

	usecs = time_to_unix(t.tm_year, t.tm_mon, t.tm_mday,
			     t.tm_hour, t.tm_min, t.tm_sec);
	tv->tv_sec = do_div(usecs, 1000000);
	tv->tv_usec = 0;
	
	return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
	return 0;
}

uint32_t _nr_syscalls = 8;
static void *_syscalls[] = {
	putstr,
	open,
	read,
	write,
	close,
	exit,
	gettimeofday,
	settimeofday,
};

void init_syscalls()
{
	/* Register our syscall handler */
	register_interrupt_handler(0x80, &_syscall_hook, syscall_handler);
}

void syscall_handler(struct registers *regs)
{
	int rc;
	void *location;

	/* Firstly, check if the requested syscall number is valid.
	 * The syscall number is found in EAX.
	 */
	if (regs->eax >= _nr_syscalls)
		return;

	location = _syscalls[regs->eax];

	/* We don't know how many parameters the function wants, so we just
	 * push them all onto the stack in the correct order. The function
	 * will use all the parameters it wants, and we can pop them all back
	 * off afterwards.
	 */
	asm volatile(" \
		     push %1; \
		     push %2; \
		     push %3; \
		     push %4; \
		     push %5; \
		     call *%6; \
		     pop %%ebx; \
		     pop %%ebx; \
		     pop %%ebx; \
		     pop %%ebx; \
		     pop %%ebx; \
		     " : "=a" (rc) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (location));
	
	regs->eax = rc;
}
