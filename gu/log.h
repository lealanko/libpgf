#ifndef GU_LOG_H_
#define GU_LOG_H_

#include <stdarg.h>
#include <gu/print.h>

typedef enum GuLogKind {
	GU_LOG_KIND_ENTER,
	GU_LOG_KIND_EXIT,
	GU_LOG_KIND_DEBUG,
	GU_LOG_KIND_WARNING,
	GU_LOG_KIND_ERROR
} GuLogKind;

void
gu_log_full(GuLogKind kind, const char* func, const char* file, int line,
	    const char* fmt, ...);

void
gu_log_full_v(GuLogKind kind, const char* func, const char* file, int line,
	      const char* fmt, va_list args);

void
gu_plog_full(GuLogKind kind, const char* func, const char* file, int line,
	     GuFmt* fmt, const void** fargs);

#ifndef NDEBUG

#define gu_logv(kind_, fmt_, args_)					\
	gu_log_full_v(kind_, __func__, __FILE__, __LINE__, fmt_, args_)

#define gu_log(kind_, ...)			\
	gu_log_full(kind_, __func__, __FILE__, __LINE__, __VA_ARGS__)

#define gu_plog(KIND, FMT, ...)			\
	gu_with_fmt_(GU_A(FMT), GU_B(__VA_ARGS__), gu_fmt_, gu_fargs_,	\
		     gu_plog_full(KIND, __func__, __FILE__, __LINE__,	\
				  &gu_fmt_, gu_fargs_))

#else

static inline void
gu_logv(GuLogKind kind, const char* fmt, va_list args)
{
	(void) kind;
	(void) fmt;
	(void) args;
}

static inline void
gu_log(GuLogKind kind, const char* fmt, ...)
{
	(void) kind;
	(void) fmt;
}

static inline void
gu_log(GuLogKind kind, const char* fmt, ...)
{
	(void) kind;
	(void) fmt;
}

#define gu_plog(KIND, FMT, ...)		\
	GU_NOP

#endif




#define gu_enter(...)			\
	gu_log(GU_LOG_KIND_ENTER, __VA_ARGS__)

#define gu_exit(...)				\
	gu_log(GU_LOG_KIND_EXIT, __VA_ARGS__)

#define gu_warn(...)				\
	gu_log(GU_LOG_KIND_WARNING, __VA_ARGS__)

#define gu_debug(...)				\
	gu_log(GU_LOG_KIND_DEBUG, __VA_ARGS__)

#define gu_pdebug(FMT, ...)			\
	gu_plog(GU_LOG_KIND_DEBUG, GU_A(FMT), __VA_ARGS__)

#define gu_debugv(kind_, fmt_, args_) \
	gu_logv(GU_LOG_KIND_DEBUG, fmt_, args_)

#endif // GU_LOG_H_
