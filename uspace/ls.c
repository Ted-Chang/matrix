#include <types.h>
#include <stddef.h>
#include <sys/stat.h>
#include <syscall.h>
#include <errno.h>
#include <dirent.h>
#include <matrix/matrix.h>

static int list_directory(int fd, char *path);
static char *mode_to_str(uint32_t mode, char *str, size_t len);

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
				printf("%s %s\n", mode_to_str(st.st_mode, str, BUF_SIZE), d.name);
			}

			close(fd_tmp);
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
