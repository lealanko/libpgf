#ifndef GU_LOG_H_
#define GU_LOG_H_

#include <stdarg.h>

typedef enum GuLogKind {
	GU_LOG_KIND_ENTER,
	GU_LOG_KIND_EXIT,
	GU_LOG_KIND_DEBUG,
	GU_LOG_KIND_ERROR
} GuLogKind;

void
gu_log_full(GuLogKind kind, const char* func, const char* file, int line,
	    const char* fmt, ...);


void
gu_log_full_v(GuLogKind kind, const char* func, const char* file, int line,
	      const char* fmt, va_list args);

#ifdef GU_LOG_ENABLE

#define gu_log(kind_, ...)			\
	gu_log_full(kind_, __func__, __FILE__, __LINE__, __VA_ARGS__)

#else

#define gu_log(kind_, ...)			\
	((void) 0)

#endif

#define gu_enter(...)			\
	gu_log(GU_LOG_KIND_ENTER, __VA_ARGS__)

#define gu_exit(...)				\
	gu_log(GU_LOG_KIND_EXIT, __VA_ARGS__)

#define gu_debug(...)				\
	gu_log(GU_LOG_KIND_DEBUG, __VA_ARGS__)



#endif // GU_LOG_H_
