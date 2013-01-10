#ifndef __SYS_MOUNT_H__
#define __SYS_MOUNT_H__

extern int mount(const char *src, const char *dir, const char *fstype,
		 int flags, const void *data);
extern int umount(const char *src);

#endif	/* __SYS_MOUNT_H__ */
