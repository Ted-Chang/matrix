#ifndef __DIRENT_H__
#define __DIRENT_H__

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* Directory entry */
struct dirent {
	ino_t d_ino;			// Inode number
	off_t d_off;			// Offset to the next dirent
	unsigned short d_reclen;	// length of this record
	unsigned char d_type;		// Type of file; not supported by
					// all file system
	char d_name[256];		// File name
};

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* __DIRENT_H__ */
