/*
 * syscall.c
 */
#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "sys/time.h"
#include "hal/isr.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "util.h"
#include "dirent.h"
#include "sys/stat.h"
#include "proc/process.h"
#include "div64.h"
#include "debug.h"
#include "fd.h"
#include "timer.h"
#include "semaphore.h"
#include "pit.h"
#include "platform.h"

#define MAX_HOSTNAME_LEN	256
#define NR_SYSCALLS		(sizeof(_syscalls)/sizeof(_syscalls[0]))

static void syscall_handler(struct registers *regs);

static struct irq_hook _syscall_hook;
static char _hostname[MAX_HOSTNAME_LEN + 1];

extern int do_unit_test(uint32_t round);

int do_null()
{
	return 0;
}

int do_exit(int rc)
{
	process_exit(rc);
	return rc;
}

int do_putstr(const char *str)
{
	putstr(str);
	return 0;
}

int do_open(const char *file, int flags, int mode)
{
	int fd = -1, rc = 0;
	struct vfs_node *n;

	/* Lookup file system node, if success the node will be referenced */
	n = vfs_lookup(file, -1);
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

int do_close(int fd)
{
	int rc = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		rc = -1;
		goto out;
	}
	
	rc = vfs_close(n);
	if (rc != 0) {
		goto out;
	}

	/* Detach the file descriptor from current process */
	rc = fd_detach(NULL, fd);

 out:
	return rc;
}

int do_read(int fd, char *buf, int len)
{
	uint32_t out = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		return -1;
	}

	out = vfs_read(n, 0, len, (uint8_t *)buf);

	return out;
}

int do_write(int fd, char *buf, int len)
{
	uint32_t out = -1;
	struct vfs_node *n;

	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		return -1;
	}

	out = vfs_write(n, 0, len, (uint8_t *)buf);
	
	return out;
}

int do_gettimeofday(struct timeval *tv, struct timezone *tz)
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

int do_settimeofday(const struct timeval *tv, const struct timezone *tz)
{
	return 0;
}

int do_readdir(int fd, int index, struct dirent *entry)
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
	kfree(e);
	rc = 0;

 out:
	return rc;
}

int do_lseek(int fd, int offset, int whence)
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

int do_lstat(int fd, void *stat)
{
	int rc = -1;
	struct vfs_node *n;
	struct stat *s;
	uint32_t flags = 0;

	if (!stat) {
		goto out;
	}
	
	n = fd_2_vfs_node(NULL, fd);
	if (!n) {
		DEBUG(DL_DBG, ("invalid fd(%d)\n", fd));
		goto out;
	}

	s = (struct stat *)stat;
	if (n->type == VFS_FILE)
		flags = S_IFREG;
	if (n->type == VFS_DIRECTORY)
		flags = S_IFDIR;
	if (n->type == VFS_PIPE)
		flags = S_IFIFO;
	if (n->type == VFS_CHARDEVICE)
		flags = S_IFCHR;
	if (n->type == VFS_BLOCKDEVICE)
		flags = S_IFBLK;
	if (n->type == VFS_SYMLINK)
		flags = S_IFLNK;

	s->st_dev = 0;
	s->st_ino = n->inode;
	s->st_mode = n->mask | flags;
	s->st_nlink = 0;
	s->st_uid = n->uid;
	s->st_gid = n->gid;
	s->st_rdev = 0;
	s->st_size = n->length;

	rc = 0;
	
 out:
	return rc;
}

int do_chdir(const char *path)
{
	int rc = -1;
	struct vfs_node *n;

	if (!path) {
		goto out;
	}

 out:
	return rc;
}

int do_mkdir(const char *path, uint32_t mode)
{
	int rc = -1;
	struct vfs_node *n;

	if (!path) {
		goto out;
	}

	n = vfs_lookup(path, VFS_DIRECTORY);
	if (n) {
		goto out;
	}

	/* Create the directory */
	rc = vfs_create(path, VFS_DIRECTORY, &n);
	if (rc != 0) {
		DEBUG(DL_WRN, ("vfs_create failed, path(%s) error(%d).\n",
			       path, rc));
	}

 out:
	return rc;
}

int do_gethostname(char *name, size_t len)
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

int do_sethostname(const char *name, size_t len)
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

int do_getuid()
{
	return CURR_PROC->uid;
}

int do_setuid(uid_t uid)
{
	int rc = -1;

	CURR_PROC->uid = uid;
	rc = 0;
	
	return rc;
}

int do_getgid()
{
	return CURR_PROC->gid;
}

int do_setgid(gid_t gid)
{
	int rc = -1;

	CURR_PROC->gid = gid;
	rc = 0;

	return rc;
}

int do_getpid()
{
	return process_getid();
}

int do_sleep(uint32_t ms)
{
	timer_delay(ms);

	return 0;
}

static char **alloc_args(const char *argv[])
{
	char **ret = NULL;
	size_t i, size, count;
	char *ptr;

	for (i = 0, size = 0; argv[i] != NULL; i++) {
		size += strlen(argv[i]) + 1;
	}

	/* All the strings plus pointers to them and a NULL pointer */
	count = i;
	size += (sizeof(char *) * (count + 1));

	ret = kmalloc(size, 0);
	if (!ret) {
		goto out;
	}

	for (i = 0, ptr = (char *)&ret[count + 1]; i < count; i++) {
		strcpy(ptr, argv[i]);
		ret[i] = ptr;
		ptr += strlen(ptr) + 1;
	}
	ret[i] = NULL;

 out:
	return ret;
}

static void free_args(char **args)
{
	kfree(args);
}

int do_create_process(const char *path, const char *args[], int flags, int priority)
{
	int rc = -1;
	char **arguments = NULL;
	struct process *p;

	if (!path || !args) {
		DEBUG(DL_DBG, ("invalid arguments.\n"));
		goto out;
	}

	/* Copy the arguments to kernel memory */
	arguments = alloc_args(args);
	if (!args) {
		DEBUG(DL_INF, ("allocate args failed.\n"));
		goto out;
	}

	p = NULL;
	
	/* By default we all use kernel process as the parent process */
	rc = process_create((const char **)arguments, _kernel_proc, 0, 16, &p);
	if (rc != 0) {
		DEBUG(DL_DBG, ("process_create failed, err(%d).\n", rc));
		goto out;
	}

	rc = p->id;

 out:
	if (arguments) {
		free_args(arguments);
	}
	
	return rc;
}

int do_waitpid(int pid)
{
	int rc = -1;
	struct process *proc;
	struct semaphore s;
	
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

	semaphore_init(&s, "waitpid-sem", 0);

	/* Put the current process into the wait queue of proc */
	rc = process_wait(proc, &s);
	if (rc != 0) {
		DEBUG(DL_INF, ("process_wait failed, proc(%p).\n", proc));
		goto out;
	}

	/* Make current thread goto sleep */
	semaphore_down(&s);

	rc = proc->status;

 out:
	return rc;
}

int do_clear()
{
	clear_scr();
	return 0;
}

int do_shutdown()
{
	platform_shutdown();
	return 0;
}

int do_syslog(char *buf, size_t len)
{
	return -1;
}

int do_mount(const char *src, const char *target, const char *fstype,
	     int flags, const void *data)
{
	int rc = -1;

	if (!target || (!src && !fstype)) {
		goto out;
	}

	/* Mount src to target with File System type */
	rc = vfs_mount(src, target, fstype, data);
	if (rc != 0) {
		DEBUG(DL_DBG, ("vfs_mount failed, type(%s), err(%d).\n",
			       fstype, rc));
		goto out;
	}

 out:
	return rc;
}

int do_umount(const char *path)
{
	int rc = -1;

	return rc;
}

/*
 * NOTE: When adding a system call, please add the following items:
 *   [1] _syscalls - the array which contains pointers to the system calls
 *   [2] define macro in /sdk/syscalls.c
 *   [3] define macro in /sdk/include/syscall.h
 */
static void *_syscalls[] = {
	do_null,
	do_exit,
	do_putstr,
	do_open,
	do_read,
	do_write,
	do_close,
	do_gettimeofday,
	do_settimeofday,
	do_readdir,
	do_lseek,
	do_lstat,
	do_chdir,
	do_mkdir,
	do_gethostname,
	do_sethostname,
	do_getuid,
	do_setuid,
	do_getgid,
	do_setgid,
	do_getpid,
	do_sleep,
	do_create_process,
	do_waitpid,
	do_unit_test,
	do_clear,
	do_shutdown,
	do_syslog,
	do_mount,
	do_umount,
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
	if (syscall_id >= NR_SYSCALLS) {
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
