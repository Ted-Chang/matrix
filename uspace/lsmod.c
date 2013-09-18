#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

static void usage();
static void list_modules();

int main(int argc, char **argv)
{
	int rc = -1;

	if (argc > 1) {
		usage();
		goto out;
	}

	list_modules();

 out:
	return rc;
}

void list_modules()
{
	;
}

void usage()
{
	printf("usage: lsmod\n");
}
