/*
 * syscall.c
 */

#include <syscall.h>

DEFN_SYSCALL1(putstr, 0, const char *)
DEFN_SYSCALL3(open, 1, const char *, int, int)
DEFN_SYSCALL3(read, 2, int, char *, int)
DEFN_SYSCALL3(write, 3, int, char *, int)
DEFN_SYSCALL1(close, 4, int)
