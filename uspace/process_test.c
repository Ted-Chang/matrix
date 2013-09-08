#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>
#include <errno.h>

void idle_spin()
{
	uint32_t cnt = 5000;

	while (cnt > 0) {
		cnt--;
	}
}

int main(int argc, char **argv)
{
	int rc = 0;
	uint32_t nr_prints = 0;

	if (argc != 2) {
		rc = -1;
		goto out;
	}

	while (TRUE) {
		printf("%s: say hello!\n", argv[1]);
		idle_spin();
		nr_prints++;
		
		/* We only do this test for 5000 round */
		if (nr_prints > 1000) {
			break;
		}
	}

 out:
	return rc;
}
