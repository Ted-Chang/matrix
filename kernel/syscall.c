#include <types.h>
#include "hal.h"
#include "syscall.h"
#include "isr.h"	// register_interrupt_handler
#include "util.h"	// putstr

static void syscall_handler(struct registers *regs);

/* Define your system call here */
DEFN_SYSCALL1(putstr, 0, const char *);
/* End of the system call definition */

static void *_syscalls[3] = {
	putstr,
};

uint32_t _nr_syscalls = 1;
static struct irq_hook _syscall_hook;

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
