// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

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

#if !defined(NDEBUG) && !defined(GU_OPTIMIZE_SIZE)

#define gu_logv(kind_, fmt_, args_)					\
	gu_log_full_v(kind_, __func__, __FILE__, __LINE__, fmt_, args_)

#define gu_log(kind_, ...)			\
	gu_log_full(kind_, __func__, __FILE__, __LINE__, __VA_ARGS__)

#define gu_plog(KIND, FMT, ...)			\
	gu_with_fmt_(GU_A(FMT), GU_B(__VA_ARGS__), gu_fmt_, gu_fargs_,	\
		     gu_plog_full(KIND, __func__, __FILE__, __LINE__,	\
				  &gu_fmt_, gu_fargs_))

#else

#define gu_logv(kind, fmt, args) GU_NOP
#define gu_log(kind, fmt, ...) GU_NOP
#define gu_plog(kind, fmt, ...) GU_NOP

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
