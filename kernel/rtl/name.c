#include <types.h>
#include <stddef.h>
#include "kstrdup.h"
#include "rtl/fsrtl.h"

int split_path(const char *path, char **dir, char **file, int flags)
{
	int rc = -1;
	size_t pos;

	if (!dir && !file) {
		goto out;
	}

	/* Find the separator */
	pos = strlen(path);
	while (pos) {
		if (path[pos - 1] == '/') {
			break;
		}
		pos--;
	}

	/* Copy the directory */
	if (dir) {
		/* Dir will contain `/' */
		*dir = kstrndup(path, pos, flags);
		if (!(*dir)) {
			goto out;
		}
	}

	/* Copy the file name */
	if (file) {
		/* Do not include `/' */
		*file = kstrdup(&path[pos], flags);
		if (!(*file)) {
			goto out;
		}
	}

	rc = 0;
	
 out:
	if (rc != 0) {
		if (dir && *dir) {
			kfree(*dir);
			*dir = NULL;
		}
		if (file && *file) {
			kfree(*file);
			*file = NULL;
		}
	}
	return rc;
}
