#ifndef __STAT_H__
#define __STAT_H__

#define _IFCHR	0020000	// character special
#define _IFDIR	0040000	// directory
#define _IFBLK	0060000	// block special
#define _IFREG	0100000	// regular
#define _IFLNK	0120000	// symbolic link
#define _IFIFO	0010000	// FIFO

/* stat structure describe file descriptor */
struct stat {
	uint16_t st_dev;
	uint16_t st_ino;
	uint32_t st_mode;
	uint16_t st_nlink;
	uint16_t st_uid;
	uint16_t st_gid;
	uint16_t st_rdev;
	uint32_t st_size;
};

#endif	/* __STAT_H__ */
