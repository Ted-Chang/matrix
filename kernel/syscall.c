/*
 * syscall.c
 */
#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <string.h>
#include "hal.h"
#include "syscall.h"
#include "isr.h"	// register_interrupt_handler
#include "util.h"	// putstr
#include "proc/process.h"
#include "matrix/matrix.h"
#include "div64.h"	// do_div
#include "matrix/debug.h"

static void syscall_handler(struct registers *regs);

static struct irq_hook _syscall_hook;

int open(const char *file, int flags, int mode)
{
	int fd = -1, rc = 0;
	struct vfs_node *n;

	/* Lookup file system node */
	n = vfs_lookup(file, 0);
	DEBUG(DL_DBG, ("open: file(%s), n(0x%x)\n", file, n));
	if (!n && FLAG_ON(flags, 0x600)) {

		DEBUG(DL_DBG, ("open: %s not found, create it.\n", file));
		
		/* The file was not found, make one */
		rc = vfs_create(file, VFS_FILE, &n);
		if (rc != 0) {
			DEBUG(DL_WRN, ("vfs_create failed, path:%s, error:%d\n",
				       file, rc));
		}
	} 

	if (n) {
		/* Attach the file descriptor to the process */
		fd = fd_attach((struct process *)CURR_PROC, n);
	}

	return fd;
}

int close(int fd)
{
	int rc = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(CURR_PROC, fd);
	if (!n) {
		rc = -1;
		goto out;
	}
	
	vfs_close(n);

	rc = fd_detach(NULL, fd);

out:
	return rc;
}

int read(int fd, char *buf, int len)
{
	uint32_t out = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(CURR_PROC, fd);
	if (!n)
		return -1;

	out = vfs_read(n, 0, len, buf);

	return out;
}

int write(int fd, char *buf, int len)
{
	uint32_t out = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(CURR_PROC, fd);
	if (!n)
		return -1;

	out = vfs_write(n, 0, len, buf);
	
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

int readdir(int fd, int index, struct dirent *entry)
{
	int rc = -1;
	struct vfs_node *n;
	struct dirent *e;

	if (!entry)
		goto out;
	
	n = fd_2_vfs_node(CURR_PROC, fd);
	if (!n) {
		DEBUG(DL_DBG, ("readdir: invalid fd(%d)\n", fd));
		goto out;
	}

	e = vfs_readdir(n, index);
	if (!e) {
		DEBUG(DL_DBG, ("readdir: fd(%d), no entry\n", fd));
		goto out;
	}

	memcpy(entry, e, sizeof(struct dirent));
	rc = 0;

	DEBUG(DL_DBG, ("readdir: index(%d), entry name(%s)\n", index, entry->name));
	// TODO: Need to make a convention whether the directory entry
	// will be freed by us.
out:
	return rc;
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
	readdir,
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
