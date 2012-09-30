#include <types.h>
#include <stddef.h>
#include <syscall.h>

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

	rc = execve("/crond", NULL, NULL);
	if (rc == -1) {
		printf("init: execute crond failed.\n");
	} else {
		printf("init: crond start successfully.\n");
	}
	
	while (TRUE) {
		sleep(1000);
	}

	return rc;
}
