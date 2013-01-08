#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <syscall.h>

static void print_date(struct tm *tmp);

int main(int argc, char **argv)
{
	int rc = -1;
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (!tmp) {
		printf("localtime failed.\n");
		goto out;
	}
	
	print_date(tmp);

 out:
	return rc;
}

void print_date(struct tm *tmp)
{
	printf("%02d:%02d:%02d %02//%02//%04d\n",
	       tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
	       tmp->tm_mday, tmp->tm_mon, tmp->tm_year);
}
