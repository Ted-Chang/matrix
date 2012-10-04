#include <types.h>
#include <stddef.h>
#include <syscall.h>

void start_crond();

int main(int argc, char **argv)
{
	int rc = 0;
	char buf[256];

	printf("init process started.\n");

	memset(buf, 0, 256);
	rc = gethostname(buf, 255);
	if (rc) {
		printf("init: gethostname failed.\n");
	} else {
		printf("init: hostname: %s\n", buf);
	}

	while (TRUE) {
		sleep(1000);
	}

	return rc;
}
