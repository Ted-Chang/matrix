#ifndef __DIRENT_H__
#define __DIRENT_H__

/* Directory entry */
struct dirent {
	char name[128];	// File name
	uint32_t ino;	// Inode number
};

#endif	/* __DIRENT_H__ */
