#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <sys/stat.h>

extern int lseek(int fd, int offset, int whence);
extern int lstat(int fd, struct stat *s);
extern int chdir(const char *path);
extern int gethostname(char *name, size_t len);
extern int close(int fd);
extern int getuid();
extern int setudi(uid_t uid);
extern int getgid();
extern int setgid(gid_t gid);

#endif	/* __UNISTD_H__ */
