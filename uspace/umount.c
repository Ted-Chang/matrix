#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <sys/mount.h>
#include <errno.h>

static void usage();

int main(int argc, char **argv)
{
	int rc = -1;

	if (argc > 1) {
		if (strcmp(argv[1], "-h") == 0) {
			usage();
			goto out;
		} else if (strcmp(argv[1], "-V")) {
			printf("umount from Matrix 0.0.1.\n");
			goto out;
		}
	}

 out:
	return rc;
}

void usage()
{
	printf("usage: umount -V	: print version\n"
	       "       umount -h	: print this help\n");
}
