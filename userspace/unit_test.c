#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int rc = 0;

	printf("unit_test process started.\n");

	do {
		rc = unit_test();
		if (rc != 0) {
			printf("unit_test failed.\n");
		} else {
			printf("unit_test finished.\n");
		}
		
	} while (FALSE);

	return rc;
}
