#ifndef __ERRNO_H__
#define __ERRNO_H__

#ifdef __KERNEL__
#define _SIGN	-
#else
#define _SIGN
#endif	/* __KERNEL__ */

extern int errno;

#define _NERROR		70		/* Number of errors */

#define EGENERIC	(_SIGN 99)	/* Generic error */
#define EPERM		(_SIGN  1)	/* Operation not permitted */
#define ENOENT		(_SIGN  2)	/* No such file or directory */
#define ESRCH		(_SIGN  3)	/* No such process */
#define EINTR		(_SIGN	4)	/* Interrupted function call */
#define EIO		(_SIGN  5)	/* Input/output error */
#define ENXIO		(_SIGN  6)	/* No such device or address */
#define E2BIG		(_SIGN  7)	/* Arg list too long */
#define ENOEXEC		(_SIGN  8)	/* Exec format error */
#define EBADF		(_SIGN  9)	/* Bad file descriptor */
#define ECHILD		(_SIGN 10)	/* No child process */
#define EAGAIN		(_SIGN 11)	/* Resource temporarily unavailable */
#define ENOMEM		(_SIGN 12)	/* Not enough space */
#define EACCESS		(_SIGN 13)	/* Permission denied */
#define EFAULT		(_SIGN 14)	/* Bad address */
#define ENOTBLK		(_SIGN 15)	/* Extension: not a block special file */
#define EBUSY		(_SIGN 16)	/* Resource busy */
#define EEXIST		(_SIGN 17)	/* File exists */
#define EXDEV		(_SIGN 18)	/* Improper link */
#define ENODEV		(_SIGN 19)	/* No such device */
#define ENOTDIR		(_SIGN 20)	/* Not a directory */
#define EISDIR		(_SIGN 21)	/* Is a directory */
#define EINVAL		(_SIGN 22)	/* Invalid argument */
#define ENFILE		(_SIGN 23)	/* Too many open files in system */
#define EMFILE		(_SIGN 24)	/* Too many open files */
#define ENOTTY		(_SIGN 25)	/* Inappropriate I/O control system */
#define ETXTBSY		(_SIGN 26)	/* No longer used */
#define EFBIG		(_SIGN 27)	/* File too large */
#define ENOSPC		(_SIGN 28)	/* No space left on device */
#define ESPIPE		(_SIGN 29)	/* Invalid seek */
#define EROFS		(_SIGN 30)	/* Read-only file system */
#define EMLINK		(_SIGN 31)	/* Too many links */
#define EPIPE		(_SIGN 32)	/* Broken pipe */
#define EDOM		(_SIGN 33)	/* Domain error */
#define ERANGE		(_SIGN 34)	/* Result too large */
#define EDEADLK		(_SIGN 35)	/* Resource deadlock avoided */
#define ENAMETOOLONG	(_SIGN 36)	/* File too long */
#define ENOLCK		(_SIGN 37)	/* No locks available */
#define ENOSYS		(_SIGN 38)	/* Function not implemented */
#define ENOTEMPTY	(_SIGN 39)	/* Directory not empty */
#define EMAPPED		(_SIGN 40)	/* Address already mapped */

#endif	/* __ERRNO_H__ */
