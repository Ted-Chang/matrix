#ifndef __STATUS_H__
#define __STATUS_H__

typedef int status_t;

#define STATUS_SUCCESS		0x00000000L
#define STATUS_NO_MEMORY	0x80000001L
#define STATUS_PROCESS_LIMIT	0x80000002L
#define STATUS_THREAD_LIMIT	0x80000003L

#endif	/* __STATUS_H__ */
