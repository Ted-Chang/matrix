#include <types.h>
#include <stddef.h>
#include <syscall.h>

void main(int argc, char **argv)
{
	int rc = 0;
	
	while (TRUE) {
		mtx_putstr("Hello from init\n");
		
		mtx_sleep(1000);
	}

	exit(rc);
}
