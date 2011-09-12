#ifndef GU_ASSERT_H_
#define GU_ASSERT_H_

#include <gu/defs.h>

typedef enum {
	GU_ASSERT_PRECOND,
	GU_ASSERT_ASSERTION,
	GU_ASSERT_POSTCOND,
	GU_ASSERT_NEVER
} GuAssertMode;

void
gu_abort_(GuAssertMode mode, const char* msg, 
	  const char* file, const char* func, int line);

#ifndef NDEBUG
#define gu_assertion_(mode_, expr_, msg_) \
	GU_BEGIN							\
	if (!(expr_)) {							\
		gu_abort_(mode_, msg_, __FILE__, __func__, __LINE__);	\
	}								\
	GU_END
#else
#define gu_assertion_(mode_, expr_, msg_) GU_NOP
#endif


#define gu_require(expr) gu_assertion_(GU_ASSERT_PRECOND, expr, #expr)
#define gu_assert(expr) gu_assertion_(GU_ASSERT_ASSERTION, expr, #expr)
#define gu_ensure(expr) gu_assertion_(GU_ASSERT_POSTCOND, expr, #expr)
#define gu_impossible() gu_assertion_(GU_ASSERT_ASSERTION, false, NULL)

void 
gu_fatal(const char* fmt, ...);

#endif /* GU_ASSERT_H_ */
