#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>
#include <sys/stat.h>
#include <errno.h>

static void usage();

int main(int argc, char **argv)
{
	int rc = -1;
	char *path;

	if (argc < 2) {
		usage();
		goto out;
	}

	path = argv[1];

	rc = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
	if (rc != 0) {
		printf("mkdir failed, err(%d).\n", rc);
		goto out;
	}

 out:
	return rc;
}

void usage()
{
	printf("usage: mkdir [option]... [directory]...\n");
}
