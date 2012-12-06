#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>
#include "matrix/matrix.h"

static void announce();
static void start_crond();
static void start_unit_test();

int main(int argc, char **argv)
{
	int rc = 0;
	char buf[256];
	uid_t uid;
	gid_t gid;

	printf("init: process started.\n");

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
	start_crond();
	
	/* Start the unit_test program */
	start_unit_test();
	
	while (TRUE) {
		printf("init: sleeping...\n");
		sleep(1000);
	}

	return rc;
}

void start_crond()
{
	int rc, status;
	char *crond[] = {
		"/crond",
		"-d",
		NULL
	};

	rc = create_process(crond[0], crond, 0, 16);
	if (rc == -1) {
		printf("create_process failed, err(%d).\n", rc);
		goto out;
	}
	printf("crond started, process id(%d).\n", rc);

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting process failed, err(%d).\n", rc);
		goto out;
	}
	printf("crond process terminated.\n");

 out:
	return;
}

void start_unit_test()
{
	int rc;
	char *unit_test[] = {
		"/unit_test",
		"-n",
		"100",
		NULL
	};

	rc = create_process(unit_test[0], unit_test, 0, 16);
	if (rc == -1) {
		printf("create_process failed, err(%d).\n", rc);
		goto out;
	}
	printf("unit_test started, process id(%d).\n", rc);

 out:
	return;
}

void announce()
{
	/* Display the Matrix startup banner */
	printf("\nMatrix %d.%d "
	       "Copyright(c) 2012, Ted Chang, Beijing, China.\n"
	       "Build date and time: %s, %s\n",
	       MATRIX_VERSION, MATRIX_RELEASE, __TIME__, __DATE__);
}

