#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>
#include "matrix/matrix.h"

static void announce();
static void start_crond();

int main(int argc, char **argv)
{
	int rc = 0;
	char buf[256];
	uid_t uid;
	gid_t gid;

	printf("init process started.\n");

	memset(buf, 0, 256);
	rc = gethostname(buf, 255);
	if (rc) {
		printf("init: gethostname failed.\n");
	} else {
		printf("init: hostname: %s\n", buf);
	}

	uid = getuid();
	gid = getgid();
	printf("init: uid(%d), gid(%d)\n", uid, gid);

	/* Print our banner */
	announce();

	/* Start the cron daemon */
	//start_crond();
	
	while (TRUE) {
		printf("init: sleeping...\n");
		sleep(1000);
	}

	return rc;
}

void start_crond()
{
	int pid, status;
	char *crond[] = {
		"/crond",
		NULL
	};

	pid = fork();
	if (pid == 0) {
		printf("start_crond: fork in child.\n");
		execve(crond[0], crond, NULL);
	} else {
		printf("start_crond: fork in parent.\n");
		status = 0;
		pid = waitpid(pid, &status, 0);
		printf("start_crond: waitpid pid(%d).\n", pid);
	}
}

void announce()
{
	/* Display the Matrix startup banner */
	printf("\nMatrix %d.%d "
	       "Copyright(c) 2012, Ted Chang, Beijing, China.\n"
	       "Build date and time: %s, %s\n",
	       MATRIX_VERSION, MATRIX_RELEASE, __TIME__, __DATE__);
}

