#include <types.h>
#include <stddef.h>
#include <stdlib.h>

void libc_main(void *ctx)
{
	int rc = -1;

	rc = main(0, NULL);
	exit(rc);
}
