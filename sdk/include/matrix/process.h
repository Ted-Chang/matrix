#ifndef __MTX_PROCESS_H__
#define __MTX_PROCESS_H__

struct process_args {
	size_t size;		// Total length of all args
	char **argv;		// Arguments array
	char **env;		// Environment array
	int argc;		// Argument count
};

#endif	/* __MTX_PROCESS_H__ */
