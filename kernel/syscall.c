/*
 * syscall.c
 */
#include <types.h>
#include "hal.h"
#include "syscall.h"
#include "isr.h"	// register_interrupt_handler
#include "util.h"	// putstr

static void syscall_handler(struct registers *regs);

static struct irq_hook _syscall_hook;

int open(const char *file, int flags, int mode)
{
	int fd = -1;

	return fd;
}

int close(int fd)
{
	int rc = -1;
	
	return -1;
}

int read(int fd, char *buf, int len)
{
	uint32_t out = -1;

	return out;
}

int write(int fd, char *buf, int len)
{
	uint32_t out = -1;
	
	return out;
}

uint32_t _nr_syscalls = 5;
static void *_syscalls[] = {
	putstr,
	open,
	read,
	write,
	close,
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
