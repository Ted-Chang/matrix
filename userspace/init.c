#include <types.h>
#include <stddef.h>
#include <syscall.h>

int main(int argc, char **argv)
{
	int rc = 0;

	printf("Hello from init printed by printf.\n");
	
	while (TRUE) {
		sleep(1000);
	}

	return rc;
}
