#include <types.h>
#include "syscall.h"
#include "isr.h"	// register_interrupt_handler
#include "util.h"	// putstr

static void syscall_handler(struct registers *regs);

/* Define your system call here */
DEFN_SYSCALL1(putstr, 0, const char *);

static void *syscalls[3] = {
	putstr,
};

uint32_t nr_syscalls = 1;

void init_syscalls()
{
	/* Register our syscall handler */
	register_interrupt_handler(0x80, syscall_handler);
}

void syscall_handler(struct registers *regs)
{
	int rc;
	void *location;
	
	/* Firstly, check if the requested syscall number is valid.
	 * The syscall number is found in EAX.
	 */
	if (regs->eax >= nr_syscalls)
		return;

	location = syscalls[regs->eax];

	/* We don't know how many parameters the function wants, so we just
	 * push them all onto the stack in the correct order. The function
	 * will use all the parameters it wants, and we can pop them all back
	 * off afterwards.
	 */
	asm volatile("push %1" : "=a" (rc) : "r" (regs->edi));
	asm volatile("push %0" : "=r" (regs->esi));
	asm volatile("push %0" : "=r" (regs->edx));
	asm volatile("push %0" : "=r" (regs->ecx));
	asm volatile("push %0" : "=r" (regs->ebx));
	asm volatile("call *%0" : "=r" (location));
	asm volatile("pop %ebx");
	asm volatile("pop %ebx");
	asm volatile("pop %ebx");
	asm volatile("pop %ebx");
	asm volatile("pop %ebx");

	regs->eax = rc;
}
