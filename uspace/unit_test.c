#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <errno.h>

static void usage();
static void echo_test();
static void ls_test();
static void cat_test();
static void mkdir_test();
static void date_test();
static void clear_test();
static void shutdown_test();

int main(int argc, char **argv)
{
	int rc = 0;
	int nr_round;

	if (argc < 3) {
		usage();
		goto out;
	}

	if (strcmp(argv[1], "-n") != 0) {
		usage();
		goto out;
	}

	nr_round = atoi(argv[2]);
	if (nr_round == 0) {
		usage();
		goto out;
	}
	
	do {
		rc = unit_test(nr_round);
		if (rc != 0) {
			printf("unit_test failed.\n");
		} else {
			printf("unit_test finished with round %d.\n", nr_round);
		}
		
	} while (FALSE);

	echo_test();
	
	mkdir_test();

	ls_test();

	cat_test();

	date_test();

	clear_test();

	shutdown_test();

	while (TRUE) {
		;
	}

 out:

	return rc;
}

void echo_test()
{
	int rc;
	char *echo[] = {
		"/echo",
		"Message from echo.",
		NULL
	};

	printf("unit_test echo message:\n");
	
	rc = create_process(echo[0], echo, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", echo[0], rc);
		goto out;
	}

 out:
	return;
}

void ls_test()
{
	int rc, status;
	char *ls_root[] = {
		"/ls",
		"-l",
		"/",
		NULL
	};
	char *ls_proc[] = {
		"/ls",
		"/proc",
		NULL
	};

	printf("unit_test list directory:\n");
	
	rc = create_process(ls_root[0], ls_root, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", ls_root[0], rc);
		goto out;
	}

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting %s failed, err(%d).\n", ls_root[0], rc);
	}

	rc = create_process(ls_proc[0], ls_proc, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", ls_proc[0], rc);
		goto out;
	}

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting %s failed, err(%d).\n", ls_proc[0], rc);
	}

 out:
	return;
}

void cat_test()
{
	int rc, status;
	char *cat[] = {
		"/cat",
		"/crontab",
		NULL
	};

	printf("unit_test cat file:\n");

	rc = create_process(cat[0], cat, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", cat[0], rc);
		goto out;
	}

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting %s failed, err(%d).\n", cat[0], rc);
		goto out;
	}

 out:
	return;
}

void mkdir_test()
{
	int rc, status;
	char *mkdir[] = {
		"/mkdir",
		"/test",
		NULL
	};

	printf("unit_test make directory.\n");

	rc = create_process(mkdir[0], mkdir, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", mkdir[0], rc);
		goto out;
	}

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting %s failed, err(%d).\n", mkdir[0], rc);
		goto out;
	}

 out:
	return;
}

void date_test()
{
	int rc, status;
	char *date[] = {
		"/date",
		NULL
	};

	printf("unit_test print date.\n");

	rc = create_process(date[0], date, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", date[0], rc);
		goto out;
	}

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting %s failed, err(%d).\n", date[0], rc);
		goto out;
	}

 out:
	return;
}

void clear_test()
{
	int rc, status;
	char *clear[] = {
		"/clear",
		NULL
	};

	printf("unit_test clear screen.\n");

	rc = create_process(clear[0], clear, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", clear[0], rc);
		goto out;
	}

	rc = waitpid(rc, &status, 0);
	if (rc == -1) {
		printf("waiting %s failed, err(%d).\n", clear[0], rc);
		goto out;
	}

 out:
	return;
}

void shutdown_test()
{
	int rc;
	char *shutdown[] = {
		"/shutdown",
		NULL
	};

	printf("unit_test shutdown.\n");

	rc = create_process(shutdown[0], shutdown, 0, 16);
	if (rc == -1) {
		printf("create_process(%s) failed, err(%d).\n", shutdown[0], rc);
		goto out;
	}

 out:
	return;
}

void usage()
{
	printf("unit_test -n round-number\n"
	       "Example: unit_test -n 100\n"
	       );
}
