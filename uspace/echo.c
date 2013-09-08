#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int rc = 0;

	printf("%s\n", argv[1]);
	
	return rc;
}

