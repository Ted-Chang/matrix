#ifndef __INITRD_H__
#define __INITRD_H__

#include "fs.h"

struct initrd_header {
	uint32_t nr_files;	// Number of files in the ramdisk
};

struct initrd_file_header {
	uint8_t magic;		// Magic number, for sanity check
	int8_t name[64];	// Filename
	uint32_t offset;	// Offset in the initrd that the file starts
	uint32_t length;	// Length of the file
};

extern struct vfs_type _ramfs_type;

extern int initrd_init(void);

#endif	/* __INITRD_H__ */
