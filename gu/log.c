#include <gu/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static bool
gu_log_match(const char* pat, size_t patlen, const char* str)
{
	if (patlen > 0 && pat[patlen-1] == '*') {
		return strncmp(pat, str, patlen-1) == 0;
	} else if (strlen(str) == patlen) {
		return strncmp(pat, str, patlen) == 0;
	} 
	return false;
}

static bool
gu_log_enabled(const char* id)
{
	const char* cfg = getenv("GU_LOG");
	if (cfg == NULL) {
		return false;
	}
	const char* p = cfg;
	while (true) {
		size_t len = strcspn(p, ",");
		if (gu_log_match(p, len, id)) {
			return true;
		}
		if (p[len] == '\0') {
			break;
		}
		p = &p[len + 1];
	} 
	return false;
}


void
gu_log_full_v(GuLogKind kind, const char* func, const char* file, int line,
	      const char* fmt, va_list args)
{
	(void) (kind);
	if (!gu_log_enabled(func) && !gu_log_enabled(file)) {
		return;
	}
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

