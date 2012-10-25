#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int rc = 0;

	printf("crond process started.\n");

	while (TRUE) {
		int fd;

		fd = open("/crontab", 0, 0);
		if (fd == -1) {
			printf("crond: open crontab failed.\n");
		} else {
			/* Parse crontab */
			;
		}
		
		break;
	}

	exit(rc);

	return rc;
}
