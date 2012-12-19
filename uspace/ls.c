#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>
#include <dirent.h>

static int list_directory(int fd);

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
		rc = list_directory(fd);
	}

	return rc;
}

int list_directory(int fd)
{
	int rc = 0, i;
	struct dirent d;

	i = 0;
	while (TRUE) {
		rc = readdir(fd, i, &d);
		if (rc != -1) {
			printf("%s\n", d.name);
		} else {
			break;
		}
		i++;
	}

	return rc;
}
