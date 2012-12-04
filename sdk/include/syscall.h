#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <stddef.h>

#define DECL_SYSCALL0(fn) int mtx_##fn();
#define DECL_SYSCALL1(fn, p1) int mtx_##fn(p1);
#define DECL_SYSCALL2(fn, p1, p2) int mtx_##fn(p1, p2);
#define DECL_SYSCALL3(fn, p1, p2, p3) int mtx_##fn(p1, p2, p3);
#define DECL_SYSCALL4(fn, p1, p2, p3, p4) int mtx_##fn(p1, p2, p3, p4);
#define DECL_SYSCALL5(fn, p1, p2, p3, p4, p5) int mtx_##fn(p1, p2, p3, p4, p5);

#define DEFN_SYSCALL0(fn, num) \
	int mtx_##fn() { \
	int a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	return a; \
	}

#define DEFN_SYSCALL1(fn, num, P1) \
	int mtx_##fn(P1 p1) { \
	int a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
	return a; \
	}

#define DEFN_SYSCALL2(fn, num, P1, P2) \
	int mtx_##fn(P1 p1, P2 p2) { \
	int a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
	return a; \
	}

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
	int mtx_##fn(P1 p1, P2 p2, P3 p3) { \
	int a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3)); \
	return a; \
	}

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
	int mtx_##fn(P1 p1, P2 p2, P3 p3, P4 p4) { \
	int a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
	return a; \
	}

#define DEFN_SYSCALL5(fn, num, P1, P2, P3, P4, P5) \
	int mtx_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
	int a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
	return a; \
	}

/* Declare your system call here */
DECL_SYSCALL1(putstr, const char *);
DECL_SYSCALL3(open, const char *, int, int);
DECL_SYSCALL3(read, int, char *, int);
DECL_SYSCALL3(write, int, char *, int);
DECL_SYSCALL1(close, int);
DECL_SYSCALL1(exit, int);
DECL_SYSCALL2(gettimeofday, void *, void *);
DECL_SYSCALL2(settimeofday, const void *, const void *);
DECL_SYSCALL3(readdir, int, int, void *);
DECL_SYSCALL3(lseek, int, int, int);
DECL_SYSCALL2(lstat, int, void *);
DECL_SYSCALL1(chdir, const char *);
DECL_SYSCALL2(mkdir, const char *, uint32_t);
DECL_SYSCALL2(gethostname, char *, size_t);
DECL_SYSCALL2(sethostname, const char *, size_t);
DECL_SYSCALL0(getuid);
DECL_SYSCALL1(setuid, uint32_t);
DECL_SYSCALL0(getgid);
DECL_SYSCALL1(setgid, uint32_t);
DECL_SYSCALL0(getpid);
DECL_SYSCALL1(sleep, uint32_t);
DECL_SYSCALL4(create_process, const char *,const char **, int, int);
DECL_SYSCALL1(wait, int)
DECL_SYSCALL0(unit_test);
/* System call declaration end */

#endif	/* __SYSCALL_H__ */
