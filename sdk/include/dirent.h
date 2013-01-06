#ifndef __DIRENT_H__
#define __DIRENT_H__

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* Directory entry */
struct dirent {
	uint32_t ino;	// Inode number
	char name[128];	// File name
};

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* __DIRENT_H__ */
