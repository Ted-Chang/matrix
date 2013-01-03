#ifndef __KSTRDUP_H__
#define __KSTRDUP_H__

extern char *kstrdup(const char *s, int flags);
extern char *kstrndup(const char *s, size_t n, int flags);

#endif	/* __KSTRDUP_H__ */
