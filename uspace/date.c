#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <syscall.h>

static void print_date(struct timeval *tv, struct timezone *tz);

int main(int argc, char **argv)
{
	int rc = -1;
	struct timeval tv;
	struct timezone tz;

	rc = gettimeofday(&tv, &tz);
	if (rc == -1) {
		printf("gettimeofday failed, err(%d).\n", rc);
		goto out;
	}

	print_date(&tv, &tz);

 out:
	return rc;
}

void print_date(struct timeval *tv, struct timezone *tz)
{
	printf("\n");
}
