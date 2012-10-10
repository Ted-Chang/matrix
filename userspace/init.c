#include <types.h>
#include <stddef.h>
#include <syscall.h>

void start_crond();

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

	start_crond();
	
	while (TRUE) {
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
		waitpid(pid, &status, 0);
	}
}
