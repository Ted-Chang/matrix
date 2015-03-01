#ifndef __STATIC_ASSERT_H__
#define __STATIC_ASSERT_H__

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(_expr_)					\
	typedef char __junk_##__LINE__[(_expr_) == 0 ? -1 : 1];
#endif

#endif	/* __STATIC_ASSERT_H__ */
