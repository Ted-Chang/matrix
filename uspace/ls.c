#include <types.h>
#include <stddef.h>
#include <sys/stat.h>
#include <syscall.h>
#include <errno.h>
#include <dirent.h>

static int list_directory(int fd, char *path);

int main(int argc, char **argv)
{
	int rc = 0, fd;
	char *path;

	if (argc == 1) {
		path = "/";
	} else {
		path = argv[1];
	}

	fd = open(path, 0, 0);
	if (fd == -1) {
		printf("open %s failed.\n", path);
	} else {
		rc = list_directory(fd, path);
		close(fd);
	}

	return rc;
}

int list_directory(int fd, char *path)
{
	int rc = 0, i, fd2;
	struct dirent d;
	char fullname[256];
	struct stat st;

	i = 1;
	while (TRUE) {
		rc = readdir(fd, i, &d);
		if (rc != -1) {
			snprintf(fullname, 256 - 1, "%s/%s", path, d.name);
			fd2 = open(fullname, 0, 0);
			if (fd2 == -1) {
				printf("open %s failed.\n", fullname);
			} else {
				rc = lstat(fd2, &st);
				if (rc == -1) {
					printf("lstat %s failed.\n", fullname);
				} else {
					printf("%c %s\n", S_ISDIR(st.st_mode) ? 'd' : '-', d.name);
				}

				close(fd2);
			}
		} else {
			break;
		}

		i++;
	}

	return rc;
}
