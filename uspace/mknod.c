#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void usage();
static void print_version();

int main(int argc, char **argv)
{
	int rc = -1;
	char *path;

	if (argc < 2) {
		usage();
		goto out;
	}

	if (strcmp(argv[1], "--help")) {
		usage();
		goto out;
	} else if (strcmp(argv[1], "--version")) {
		print_version();
		goto out;
	}

	path = argv[1];

	rc = mknod(path, 0, 0);
	if (rc != 0) {
		printf("mknod failed, err(%d).\n", rc);
		goto out;
	}

 out:
	return rc;
}

void usage()
{
	printf("usage: mknod [option]... name type [major minor]\n"
	       "       mknod --help	prnit this help\n"
	       "       mknod --version  print version info\n");
}

void print_version()
{
	printf("mknod (Matrix coreutils) 0.01\n");
}
