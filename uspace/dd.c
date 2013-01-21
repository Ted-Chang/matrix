#include <types.h>
#include <stddef.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

static void usage();

int main(int argc, char **argv)
{
	int rc = -1;

	if (argc > 2) {
		if (strcmp(argv[1], "--help") == 0) {
			usage();
			goto out;
		} else if (strcmp(argv[1], "--version") == 0) {
			printf("dd from Matrix 0.0.1\n");
			goto out;
		}
	}

 out:
	return rc;
}

void usage()
{
	printf("usage: dd [operator] ...\n"
	       "   or: dd option\n"
	       "avaliable options are:\n"
	       "\t--help\tdisplay this help information and exit\n"
	       "\t--version\tdisplay version information and exit\n");
}
