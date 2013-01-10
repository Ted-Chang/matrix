#ifndef __TYPES_H__
#define __TYPES_H__

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
typedef char int8_t;

typedef int bool_t;
typedef char boolean_t;

typedef unsigned long u_long;
typedef unsigned char u_char;

/* Physical address type and memory size definition */
typedef uint32_t phys_addr_t;
typedef uint32_t phys_size_t;

typedef int ptrdiff_t;

/* Micro second type definition */
typedef uint64_t useconds_t;

/* User id and group id */
typedef uint32_t uid_t;
typedef uint32_t gid_t;

/* Process id and thread id */
typedef int pid_t;
typedef int tid_t;

/* Ptr type definition */
typedef unsigned long ptr_t;

/* File stat definition */
typedef uint32_t dev_t;

#ifndef __USE_FILE_OFFSET64
typedef uint32_t ino_t;
#else
typedef uint64_t ino_t;
#endif	/* __USE_FILE_OFFSET64 */

typedef uint32_t nlink_t;
typedef uint32_t mode_t;

#ifndef __USE_FILE_OFFSET64
typedef uint32_t off_t;
#else
typedef uint64_t off_t;
#endif	/* __USE_FILE_OFFSET64 */

#endif	/* __TYPES_H__ */
