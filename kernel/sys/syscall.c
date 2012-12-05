/*
 * syscall.c
 */
#include <types.h>
#include <stddef.h>
#include <string.h>
#include "sys/time.h"
#include "hal/hal.h"
#include "hal/isr.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "util.h"
#include "dirent.h"
#include "sys/stat.h"
#include "proc/process.h"
#include "matrix/matrix.h"
#include "div64.h"
#include "matrix/debug.h"
#include "fd.h"
#include "timer.h"
#include "semaphore.h"
#include "pit.h"

#define MAX_HOSTNAME_LEN	256

static void syscall_handler(struct registers *regs);

static struct irq_hook _syscall_hook;
static char _hostname[MAX_HOSTNAME_LEN + 1];

int open(const char *file, int flags, int mode)
{
	int fd = -1, rc = 0, file_type = VFS_FILE;
	struct vfs_node *n;

	/* Lookup file system node */
	n = vfs_lookup(file, file_type);
	DEBUG(DL_DBG, ("file(%s), n(0x%x)\n", file, n));
	if (!n && FLAG_ON(flags, 0x600)) {

		DEBUG(DL_DBG, ("%s not found, create it.\n", file));
		
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

	n = fd_2_vfs_node(NULL, fd);
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

	n = fd_2_vfs_node(NULL, fd);
	if (!n)
		return -1;

	out = vfs_read(n, 0, len, (uint8_t *)buf);

	return out;
}

int write(int fd, char *buf, int len)
{
	uint32_t out = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(NULL, fd);
	if (!n)
		return -1;

	out = vfs_write(n, 0, len, (uint8_t *)buf);
	
	return out;
}

int exit(int rc)
{
	process_exit(rc);
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
	do_div(usecs, 1000000);
	tv->tv_sec = usecs;
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

	if (!entry) {
		goto out;
	}
	
	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		DEBUG(DL_DBG, ("invalid fd(%d)\n", fd));
		goto out;
	}

	e = vfs_readdir(n, index);
	if (!e) {
		DEBUG(DL_DBG, ("fd(%d), no entry\n", fd));
		goto out;
	}

	memcpy(entry, e, sizeof(struct dirent));
	rc = 0;

	// TODO: Need to make a convention whether the directory entry
	// will be freed by us.
out:
	return rc;
}

int lseek(int fd, int offset, int whence)
{
	int off = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		DEBUG(DL_DBG, ("invalid fd(%d)\n", fd));
		goto out;
	}

	switch (whence) {
	case 0:
		n->offset = offset;
		off = n->offset;
		break;
	case 1:
		n->offset += offset;
		off = n->offset;
		break;
	case 2:
		n->offset = n->length + offset;
		off = n->offset;
		break;
	default:
		DEBUG(DL_DBG, ("invalid whence(%d)\n", whence));
	}

out:
	return off;
}

int lstat(int fd, void *stat)
{
	int rc = -1;
	struct vfs_node *n;
	struct stat *s;
	uint32_t flags = 0;

	if (!stat)
		goto out;
	
	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		DEBUG(DL_DBG, ("invalid fd(%d)\n", fd));
		goto out;
	}

	s = (struct stat *)stat;
	if (n->type == VFS_FILE)
		flags = _IFREG;
	if (n->type == VFS_DIRECTORY)
		flags = _IFDIR;
	if (n->type == VFS_PIPE)
		flags = _IFIFO;
	if (n->type == VFS_CHARDEVICE)
		flags = _IFCHR;
	if (n->type == VFS_BLOCKDEVICE)
		flags = _IFBLK;
	if (n->type == VFS_SYMLINK)
		flags = _IFLNK;

	s->st_dev = 0;
	s->st_ino = n->inode;
	s->st_mode = n->mask | flags;
	s->st_nlink = 0;
	s->st_uid = n->uid;
	s->st_gid = n->gid;
	s->st_rdev = 0;
	s->st_size = n->length;
	
out:
	return rc;
}

int chdir(const char *path)
{
	int rc = -1;
	struct vfs_node *n;

	if (!path) {
		goto out;
	}

out:
	return rc;
}

int mkdir(const char *path, uint32_t mode)
{
	int rc = -1;

	if (!path) {
		goto out;
	}

out:
	return rc;
}

int gethostname(char *name, size_t len)
{
	int rc = -1;

	if (!name || !len) {
		goto out;
	}
	
	strncpy(name, _hostname, len);
	rc = 0;

out:
	return rc;
}

int sethostname(const char *name, size_t len)
{
	int rc = -1;

	if (!name || !len || len > MAX_HOSTNAME_LEN) {
		goto out;
	}

	memcpy(_hostname, name, len);
	rc = 0;

out:
	return rc;
}

int getuid()
{
	return CURR_PROC->uid;
}

int setuid(uid_t uid)
{
	int rc = -1;

	CURR_PROC->uid = uid;
	rc = 0;
	
	return rc;
}

int getgid()
{
	return CURR_PROC->gid;
}

int setgid(gid_t gid)
{
	int rc = -1;

	CURR_PROC->gid = gid;
	rc = 0;

	return rc;
}

int getpid()
{
	return process_getid();
}

int sleep(uint32_t ms)
{
	timer_delay(ms);

	return 0;
}

int create_process(const char *path, const char *args[], int flags, int priority)
{
	int rc = -1;
	struct process *p;

	if (!path) {
		DEBUG(DL_DBG, ("invalid arguments.\n"));
		goto out;
	}

	p = NULL;
	
	/* By default we all use kernel process as the parent process */
	rc = process_create(args, _kernel_proc, 0, 16, &p);
	if (rc != 0) {
		DEBUG(DL_DBG, ("process_create failed, err(%d).\n", rc));
		goto out;
	}

	rc = p->id;

out:
	return rc;
}

int waitpid(int pid)
{
	int rc = -1;
	struct process *proc;
	struct semaphore *s;
	
	if (pid < 1) {
		DEBUG(DL_DBG, ("group wait not supported, pid(%d).\n", pid));
		rc = 0;
		goto out;
	}

	/* Get the process corresponding to the process id */
	proc = process_lookup(pid);
	if (!proc) {
		DEBUG(DL_DBG, ("pid(%d) not found in process tree.\n", pid));
		goto out;
	}

	/* Allocate semaphore */
	s = kmalloc(sizeof(struct semaphore), 0);
	if (!s) {
		DEBUG(DL_DBG, ("kmalloc semaphore failed.\n"));
		goto out;
	}
	semaphore_init(s, "waitpid-sem", 0);

	/* Put the current process into the wait queue of proc */
	rc = process_wait(proc, s);
	if (rc != 0) {
		DEBUG(DL_INF, ("process_wait failed, proc(%p).\n", proc));
		goto out;
	}

	/* Make current thread goto sleep */
	semaphore_down(s);

	rc = proc->status;

 out:
	return rc;
}

void unit_test_thread(void *ctx)
{
	int j;
	uint64_t i;
	slab_cache_t ut_cache;
	void *obj[4];

	i = 0;
	slab_cache_init(&ut_cache, "ut-cache", 256, NULL, NULL, 0);
	while (TRUE) {
		kd_printf("unit-test round %lld.\n", i);
		
		for (j = 0; j < 4; j++) {
			obj[j] = slab_cache_alloc(&ut_cache);
			if (!obj[j]) {
				DEBUG(DL_INF, ("slab_cache_alloc failed.\n"));
				goto out;
			} else {
				memset(obj[j], 0, 256);
			}
		}

		for (j = 0; j < 4; j++) {
			if (obj[j]) {
				slab_cache_free(&ut_cache, obj[j]);
				obj[j] = NULL;
			}
		}
		
		i++;
	}

 out:
	for (j = 0; j < 4; j++) {
		if (obj[j]) {
			slab_cache_free(&ut_cache, obj[j]);
			obj[j] = NULL;
		}
	}
	slab_cache_delete(&ut_cache);
	
	return;
}

int unit_test()
{
	int rc = 0;

	/* Create a kernel mode unit test thread */
	rc = thread_create("unit-test", NULL, 0, unit_test_thread, NULL, NULL);
	if (rc != 0) {
		DEBUG(DL_WRN, ("thread_create unit test thread failed, err(%d).\n", rc));
		goto out;
	}

 out:
	return rc;
}

/*
 * NOTE: When adding a system call, please add the following items:
 *   [1] _nr_syscalls - number of the system calls
 *   [2] _syscalls - the array which contains pointers to the system calls
 *   [3] define macro in /loader/syscalls.c
 *   [4] define macro in /include/syscall.h
 */
uint32_t _nr_syscalls = 25;
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
	lseek,
	lstat,
	chdir,
	mkdir,
	gethostname,
	sethostname,
	getuid,
	setuid,
	getgid,
	setgid,
	getpid,
	sleep,
	create_process,
	waitpid,
	unit_test,
	NULL
};

void init_syscalls()
{
	/* Register our syscall handler */
	register_irq_handler(0x80, &_syscall_hook, syscall_handler);

	/* Initialize the hostname */
	memset(_hostname, 0, MAX_HOSTNAME_LEN + 1);
	strcpy(_hostname, "Matrix");
}

void syscall_handler(struct registers *regs)
{
	int rc;
	void *location;
	uint32_t syscall_id;

	/* Firstly, check if the requested syscall number is valid.
	 * The syscall number is found in EAX.
	 */
	syscall_id = regs->eax;
	if (syscall_id >= _nr_syscalls) {
		DEBUG(DL_WRN, ("invalid syscall(%d)", syscall_id));
		return;
	}

	location = _syscalls[syscall_id];

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
		     " : "=a"(rc) : "r"(regs->edi), "r"(regs->esi), "r"(regs->edx),
		     "r"(regs->ecx), "r"(regs->ebx), "r"(location));
	
	regs->eax = rc;
}
