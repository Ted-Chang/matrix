#include <types.h>
#include <stddef.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

static void usage();

int main(int argc, char **argv)
{
	int rc = -1;
	char *path;

	if (argc >= 2) {
		if (strcmp(argv[1], "-") == 0) {
			;
		} else if (strcmp(argv[1], "-P") == 0) {
			;
		} else if (strcmp(argv[1], "-e") == 0) {
			;
		} else if (strncmp(argv[1], "-", 1) == 0) {
			usage();
			goto out;
		} else {
			path = argv[1];
		}
	} else {
		path = "/";
	}

	rc = chdir(path);
	if (rc != 0) {
		printf("chdir failed, err(%d).\n", rc);
		goto out;
	}

 out:
	return rc;
}

void usage()
{
	printf("usage: cd [-L|[-P [-e]]] [dir]\n");
}
