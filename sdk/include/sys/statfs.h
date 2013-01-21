#ifndef __SYS_STATFS_H__
#define __SYS_STATFS_H__

struct statfs {
	int f_type;		// Type of the FS
	fsblkcnt_t f_blocks;	// Total data blocks in FS
	fsblkcnt_t f_bfree;	// Free blocks in FS
	fsfilcnt_t f_files;	// Total file node in FS
	fsfilcnt_t f_ffree;	// Free file node in FS
};

#endif	/* __SYS_STATFS_H__ */
