/*
 * syscall.c
 */

#include <syscall.h>

DEFN_SYSCALL1(putstr, 0, const char *)
DEFN_SYSCALL3(open, 1, const char *, int, int)
DEFN_SYSCALL3(read, 2, int, char *, int)
DEFN_SYSCALL3(write, 3, int, char *, int)
DEFN_SYSCALL1(close, 4, int)
DEFN_SYSCALL1(exit, 5, int)
DEFN_SYSCALL2(gettimeofday, 6, void *, void *)
DEFN_SYSCALL2(settimeofday, 7, const void *, const void *)
DEFN_SYSCALL3(readdir, 8, int, int, void *)
DEFN_SYSCALL3(lseek, 9, int, int, int)
DEFN_SYSCALL2(lstat, 10, int, void *)
DEFN_SYSCALL1(chdir, 11, const char *)
DEFN_SYSCALL2(mkdir, 12, const char *, uint32_t)
DEFN_SYSCALL3(execve, 13, const char *, const char **, const char **)
DEFN_SYSCALL0(fork, 14)
DEFN_SYSCALL2(gethostname, 15, char *, size_t)
DEFN_SYSCALL2(sethostname, 16, const char *, size_t)
DEFN_SYSCALL0(getuid, 17)
DEFN_SYSCALL1(setuid, 18, uid_t)
DEFN_SYSCALL0(getgid, 19)
DEFN_SYSCALL1(setgid, 20, gid_t)
DEFN_SYSCALL0(getpid, 21)
