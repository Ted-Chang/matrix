#ifndef __SYS_STAT_H__
#define __SYS_STAT_H__

#include <types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* File type mode definition */
#define S_IFMT		0170000	// Bitmask for the file type
#define S_IFSOCK	0140000	// Socket
#define S_IFLNK		0120000	// symbolic link
#define S_IFREG		0100000	// regular
#define S_IFBLK		0060000	// block special
#define S_IFDIR		0040000	// directory
#define S_IFCHR		0020000	// character special
#define S_IFIFO		0010000	// FIFO

/* Test macro for file types */
#define __S_ISTYPE(mode, mask)	(((mode) & S_IFMT) == (mask))

#define S_ISDIR(mode)	__S_ISTYPE((mode), S_IFDIR)
#define S_ISCHR(mode)	__S_ISTYPE((mode), S_IFCHR)
#define S_ISBLK(mode)	__S_ISTYPE((mode), S_IFBLK)
#define S_ISREG(mode)	__S_ISTYPE((mode), S_IFREG)
#define S_ISFIFO(mode)	__S_ISTYPE((mode), S_IFIFO)
#define S_ISLNK(mode)	__S_ISTYPE((mode), S_IFLNK)
#define S_ISSOCK(mode)	__S_ISTYPE((mode), S_IFSOCK)

/* Permissions */
#define S_ISUID		04000	// Set UID bit
#define S_ISGID		02000	// Set GID bit
#define S_ISVTX		01000	// Sticky bit
	
#define S_IRWXU		00700	// Mask for file owner permissions
#define S_IRUSR		00400	// Owner has read permission
#define S_IWUSR		00200	// Owner has write permission
#define S_IXUSR		00100	// Owner has execute permission
#define S_IRWXG		00070	// Mask for group permissions
#define S_IRGRP		00040	// Group has read permission
#define S_IWGRP		00020	// Group has write permission
#define S_IXGRP		00010	// Group has execute permission
#define S_IRWXO		00007	// Mask for permissions for others (not in group)
#define S_IROTH		00004	// Others have read permission
#define S_IWOTH		00002	// Others have write permission
#define S_IXOTH		00001	// Others have execute permission

/* stat structure describe file descriptor */
struct stat {
	dev_t st_dev;		// ID of device containing file
	ino_t st_ino;		// Inode number
	mode_t st_mode;		// Protection
	nlink_t st_nlink;	// Number of hard links
	uid_t st_uid;		// User ID of the owner
	gid_t st_gid;		// Group ID of the owner
	dev_t st_rdev;		// Device ID (if special file)
	off_t st_size;		// Total size, in bytes
	time_t st_atime;	// Time of last access
	time_t st_mtime;	// Time of last modification
	time_t st_ctime;	// Time of last status change
};

#ifdef __cplusplus
};
#endif	/* __cplusplus */

#endif	/* __SYS_STAT_H__ */
