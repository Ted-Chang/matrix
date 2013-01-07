#include <types.h>
#include <stddef.h>
#include <sys/stat.h>
#include <syscall.h>
#include <errno.h>
#include <dirent.h>

static int list_directory(int fd, char *path);
static char *stat_to_str(struct stat *st, char *str, size_t len);

int main(int argc, char **argv)
{
	int rc = 0, fd;
	char *path;

	if (argc == 1) {
		path = "/";
	} else {
		path = argv[1];
	}

	fd = open(path, 0, 0);
	if (fd == -1) {
		printf("open %s failed.\n", path);
	} else {
		rc = list_directory(fd, path);
		close(fd);
	}

	return rc;
}

#define BUF_SIZE	24
int list_directory(int fd, char *path)
{
	int rc = 0, i, fd_tmp;
	struct dirent d;
	char fullname[256];
	struct stat st;
	char str[BUF_SIZE];

	i = 1;
	while (TRUE) {
		rc = readdir(fd, i, &d);
		if (rc == -1) {
			break;
		}

		snprintf(fullname, 256 - 1, "%s/%s", path, d.name);

		fd_tmp = open(fullname, 0, 0);
		if (fd_tmp == -1) {
			printf("open %s failed.\n", fullname);
		} else {
			rc = lstat(fd_tmp, &st);
			if (rc == -1) {
				printf("lstat %s failed.\n", fullname);
			} else {
				printf("%s %s\n", stat_to_str(&st, str, BUF_SIZE), d.name);
			}

			close(fd_tmp);
		}

		i++;
	}

	return rc;
}

char *stat_to_str(struct stat *st, char *str, size_t len)
{
	snprintf(str, len, "%c.",
		 S_ISDIR(st->st_mode) ? 'd' : '-');

	return str;
}
