#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct initrd_header {
	unsigned char magic;
	char name[64];
	unsigned int offset;
	unsigned int length;
};


static void usage();

void main(int argc, char **argv)
{
	int rc = 0, i;
	int nr_headers;
	struct initrd_header headers[64];
	unsigned int off;
	FILE *wfp;
	unsigned char *data;

	if (argc <= 3) {
		usage();
		goto out;
	}

	nr_headers = (argc - 2)/2;
	off = sizeof(struct initrd_header)*64 + sizeof(int);
	
	printf("size of header: %d\n", sizeof(struct initrd_header));

	for (i = 0; i < nr_headers; i++) {
		FILE *fp;
		
		printf("writing file %s->%s at 0x%x\n", argv[i*2+1], argv[i*2+2], off);
		strcpy(headers[i].name, argv[i*2+2]);
		headers[i].offset = off;

		fp = fopen(argv[i*2+1], "r");
		if (fp == 0) {
			printf("Failed to open file: %s\n", argv[i*2+1]);
			goto out;
		}

		fseek(fp, 0, SEEK_END);
		headers[i].length = ftell(fp);
		off += headers[i].length;
		fclose(fp);
		headers[i].magic = 0xBF;
	}

	wfp = fopen(argv[argc - 1], "w");
	if (wfp == 0) {
		printf("Failed to open file: %s\n", argv[argc - 1]);
		goto out;
	}

	data = (unsigned char *)malloc(off);
	fwrite(&nr_headers, sizeof(int), 1, wfp);
	fwrite(headers, sizeof(struct initrd_header), 64, wfp);

	for (i = 0; i < nr_headers; i++) {
		FILE *fp = fopen(argv[i*2+1], "r");
		unsigned char *buf = malloc(headers[i].length);

		fread(buf, 1, headers[i].length, fp);
		fwrite(buf, 1, headers[i].length, wfp);
		fclose(fp);
		free(buf);
	}

	fclose(wfp);
	free(data);

out:

	exit(rc);
}

void usage()
{
	printf("usage: makeinitrd srcpath1 dstfile1 srcpath2 dstfile2 ... imgpath\n"
	       "note: maximum number of files is 64\n"
	       "example: makeinitrd ../bin/init init ../bin/initrd\n"
	       "         makeinitrd ../bin/echo echo ../bin/ls ls ../bin/cat cat ../bin/initrd\n");
}
