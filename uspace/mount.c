#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <syscall.h>
#include <errno.h>

static void usage();
static void list_mounts();

int main(int argc, char **argv)
{
	int rc = -1;

	if (argc > 1) {
		if (strcmp(argv[1], "-h") == 0) {
			usage();
			goto out;
		} else if (strcmp(argv[1], "-V") == 0) {
			printf("mount from Matrix 0.0.1.\n");
			goto out;
		}
	} else if (argc == 1) {
		list_mounts();
	}

 out:
	return rc;
}

void list_mounts()
{
	;
}

void usage()
{
	printf("usage: mount -V		: print version\n"
	       "       mount -h		: print this help\n"
	       "       mount		: list mounted filesystems\n");
}
