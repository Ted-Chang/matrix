#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <syscall.h>
#include <errno.h>
#include <dirent.h>
#include <matrix/matrix.h>

enum ls_option {
	ls_null,	// ls
	ls_l,		// ls -l
	ls_a,		// ls -a
};

static void usage();
static int list_directory(int fd, char *path, enum ls_option opt);
static char *mode_to_str(uint32_t mode, char *str, size_t len);

int main(int argc, char **argv)
{
	int rc = 0, fd;
	enum ls_option opt = ls_null;
	char *path = NULL;

	if (argc == 1) {
		path = "/";
	} else if (strcmp(argv[1], "--help") == 0) {
		usage();
		goto out;
	} else if (strcmp(argv[1], "--version") == 0) {
		printf("ls (Matrix coreutils) 0.01\n");
		goto out;
	} else if (strcmp(argv[1], "-l") == 0) {
		opt = ls_l;
		path = argv[2];
	} else if (strncmp(argv[1], "-", 1) == 0) {
		usage();
		goto out;
	} else if (argc == 2) {
		path = argv[1];
	}
	
	fd = open(path, 0, 0);
	if (fd == -1) {
		printf("open %s failed.\n", path);
	} else {
		rc = list_directory(fd, path, opt);
		close(fd);
	}

 out:
	return rc;
}

#define BUF_SIZE	24
int list_directory(int fd, char *path, enum ls_option opt)
{
	int rc = 0, i, fd_tmp;
	struct dirent d;
	char fullname[256];
	struct stat st;
	char str[BUF_SIZE];

	i = 0;
	while (TRUE) {
		rc = readdir(fd, i, &d);
		if (rc != 0) {
			break;
		}

		switch (opt) {
		case ls_l:
			snprintf(fullname, 256 - 1, "%s/%s", path, d.d_name);

			fd_tmp = open(fullname, 0, 0);
			if (fd_tmp == -1) {
				printf("open %s failed.\n", fullname);
			} else {
				rc = lstat(fd_tmp, &st);
				if (rc == -1) {
					printf("lstat %s failed.\n", fullname);
				} else {
					printf("%s %s\n", mode_to_str(st.st_mode, str, BUF_SIZE), d.d_name);
				}

				close(fd_tmp);
			}
			break;	// Break out the switch
		case ls_null:
			printf("%s\n", d.d_name);
			break;
		default:
			printf("Unknown option!!!\n");
			break;
		}

		i++;
	}

	return rc;
}

char *mode_to_str(uint32_t mode, char *str, size_t len)
{
	char type = '-';

	if (S_ISDIR(mode)) {
		type = 'd';
	} else if (S_ISLNK(mode)) {
		type = 'l';
	} else if (S_ISBLK(mode)) {
		type = 'b';
	} else if (S_ISCHR(mode)) {
		type = 'c';
	} else if (S_ISSOCK(mode)) {
		type = 's';
	}
	
	snprintf(str, len, "%c%c%c%c%c%c%c%c%c%c.",
		 type,
		 FLAG_ON(mode, S_IRUSR) ? 'r' : '-',
		 FLAG_ON(mode, S_IWUSR) ? 'w' : '-',
		 FLAG_ON(mode, S_IXUSR) ? 'x' : '-',
		 FLAG_ON(mode, S_IRGRP) ? 'r' : '-',
		 FLAG_ON(mode, S_IWGRP) ? 'w' : '-',
		 FLAG_ON(mode, S_IXGRP) ? 'x' : '-',
		 FLAG_ON(mode, S_IROTH) ? 'r' : '-',
		 FLAG_ON(mode, S_IWOTH) ? 'w' : '-',
		 FLAG_ON(mode, S_IXOTH) ? 'x' : '-');

	return str;
}

void usage()
{
	printf("usage: ls [option]... [file]...\n"
	       "  -l		use long format list information\n"
	       "  --help	show this help information and exit\n"
	       "  --version	show version information and exit\n");
}
