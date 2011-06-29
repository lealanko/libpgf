#include "log.h"

#include <stdio.h>


void
gu_log_full_v(GuLogKind kind, const char* func, const char* file, int line,
	      const char* fmt, va_list args)
{
	(void) (kind && func);
	fprintf(stderr, "%s:%d: ", file, line);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	fflush(stderr);
}



void
gu_log_full(GuLogKind kind, const char* func, const char* file, int line,
	    const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	gu_log_full_v(kind, func, file, line, fmt, args);
	va_end(args);
}

