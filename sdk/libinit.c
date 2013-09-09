#include <types.h>
#include <stddef.h>
#include <stdlib.h>
#include "matrix/process.h"

extern int main(int argc, char **argv);

void libc_main(struct process_args *args)
{
	int rc = -1;

	rc = main(args->argc, args->argv);
	exit(rc);
}
