// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <gu/defs.h>
#include <gu/log.h>
#include <gu/file.h>
#include <gu/write.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static int gu_log_depth = 0;

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
gu_log_enabled(const char* func, const char* file)
{
	const char* cfg = getenv("GU_LOG");
	if (cfg == NULL) {
		return false;
	}
	const char* p = cfg;
	while (true) {
		size_t len = strcspn(p, ",");
		if (gu_log_match(p, len, func)) {
			return true;
		}
		if (gu_log_match(p, len, file)) {
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
	if (!gu_log_enabled(func, file)) {
		return;
	}
	if (kind == GU_LOG_KIND_EXIT) {
		gu_log_depth--;
	}
	if (fmt) {
		fprintf(stderr, "%03d:%-32s: ", gu_log_depth, func);
		vfprintf(stderr, fmt, args);
		fputc('\n', stderr);
		fflush(stderr);
	}
	if (kind == GU_LOG_KIND_ENTER) {
		gu_log_depth++;
	}
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

void
gu_plog_full(GuLogKind kind, const char* func, const char* file, int line,
	     GuFmt* fmt, const void** fargs)
{
	if (!gu_log_enabled(func, file)) {
		return;
	}
	if (kind == GU_LOG_KIND_EXIT) {
		gu_log_depth--;
	}
	
	GuPool* pool = gu_local_pool();
	GuOut* errf = gu_file_out(stderr, pool);
	GuWriter* wtr = gu_new_utf8_writer(errf, pool);
	GuExn* exn = NULL;
	gu_printf(wtr, exn, "%03d:%-32s: ", gu_log_depth, func);
	gu_print_fmt(fmt, fargs, wtr, exn);
	gu_putc('\n', wtr, exn);
	gu_writer_flush(wtr, exn);
	gu_pool_free(pool);
	if (kind == GU_LOG_KIND_ENTER) {
		gu_log_depth++;
	}
}

