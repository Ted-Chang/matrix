#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>

void idle_spin()
{
	uint32_t cnt = 20000;

	while (cnt > 0) {
		cnt--;
	}
}

int main(int argc, char **argv)
{
	int rc = 0;

	if (argc != 2) {
		rc = -1;
		goto out;
	}

	while (TRUE) {
		printf("%s: say hello!\n", argv[1]);
		idle_spin();
	}

 out:
	return rc;
}
