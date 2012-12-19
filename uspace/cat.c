#include <types.h>
#include <stddef.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

static void usage();
static int cat_file(int fd);

int main(int argc, char **argv)
{
	int rc = 0, fd;
	char *path;

	if (argc >= 2) {
		if (strcmp(argv[1], "--h") == 0) {
			usage();
			goto out;
		} else {
			path = argv[1];
		}
	} else {
		goto out;
	}

	fd = open(path, 0, 0);
	if (fd == -1) {
		printf("open %s failed.\n", path);
	} else {
		rc = cat_file(fd);
	}

 out:
	return rc;
}

int cat_file(int fd)
{
	int rc = 0;
	char buf[1024] = {0};

	rc = read(fd, buf, 1024);
	if (rc == -1) {
		printf("read failed.\n");
	} else {
		printf("%s\n", buf);
	}

	return rc;
}

void usage()
{
	printf("usage: cat [option]... [file]...\n");
}
