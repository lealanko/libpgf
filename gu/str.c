#include <gu/str.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

char* 
gu_str_new(int size, GuPool* pool)
{
	char* str = gu_new_n(pool, char, size + 1);
	memset(str, '\0', size + 1);
	return str;
}

char*
gu_strdup(const char* cstr, GuPool* pool)
{
	int len = strlen(cstr);
	char* str = gu_str_new(len, pool);
	memcpy(str, cstr, len);
	return str;
}

static bool
gu_str_eq_fn(GuEqFn* self, const void* p1, const void* p2)
{
	(void) self;
	const GuStr* sp1 = p1;
	const GuStr* sp2 = p2;
	return (strcmp(*sp1, *sp2) == 0);
}

static unsigned
gu_str_hash_fn(GuHashFn* self, const void* p)
{
	(void) self;
	const GuStr* sp = p;
	unsigned h = 0;
	for (char* s = *sp; *s != '\0'; s++) {
		h = 101 * h + (unsigned char) *s;
	}
	return h;
}

GuHashFn gu_str_hash = { gu_str_hash_fn };
GuEqFn gu_str_eq = { gu_str_eq_fn };

GU_DEFINE_TYPE(GuStr, repr, _);


wchar_t* 
gu_wcs_new(int size, GuPool* pool)
{
	wchar_t* wcs = gu_new_n(pool, wchar_t, size + 1);
	wmemset(wcs, L'\0', size + 1);
	return wcs;
}

wchar_t*
gu_wcsdup(const wchar_t* wcs, GuPool* pool)
{
	int len = wcslen(wcs);
	wchar_t* dup = gu_wcs_new(len, pool);
	wmemcpy(dup, wcs, len);
	return dup;
}


/* vswprintf, unlike vsnprintf, cannot precalculate the required
   length directly, so we have to do it by trying successively larger
   buffers */
static int
gu_vswprintf_len(const wchar_t* fmt, va_list args)
{
	int len = -1;
	wchar_t dummy[64];
	len = vswprintf(dummy, 64, fmt, args);
	if (len >= 0) {
		return len;
	}
	size_t size = 64 * sizeof(wchar_t);
	wchar_t* buf = NULL;
	while (len < 0) {
		if (size == SIZE_MAX) {
			break;
		}
		size = size > SIZE_MAX / 2 ? SIZE_MAX : size * 2;
		buf = gu_mem_buf_realloc(buf, size * 2, &size);
		if (!size) {
			break;
		}
		size_t maxlen = size / sizeof(wchar_t);
		va_list args_copy;
		va_copy(args_copy, args);
		len = vswprintf(buf, maxlen, fmt, args_copy);
		va_end(args_copy);
	}
	gu_mem_buf_free(buf);
	return len;
}

wchar_t* 
gu_vaswprintf(const wchar_t* fmt, va_list args, GuPool* pool)
{
	va_list args2;
	va_copy(args2, args);
	int len = gu_vswprintf_len(fmt, args2);
	va_end(args2);
	if (len < 0) {
		return NULL;
	}
	wchar_t* wcs = gu_wcs_new(len, pool);
	vswprintf(wcs, len + 1, fmt, args);
	return wcs;
}

wchar_t* 
gu_aswprintf(GuPool* pool, const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	wchar_t* wcs = gu_vaswprintf(fmt, args, pool);
	va_end(args);
	return wcs;
}

wchar_t*
gu_str_wcs(const char* str, GuPool* pool)
{
	size_t len = mbstowcs(NULL, str, 0);
	wchar_t* wcs = gu_wcs_new(len, pool);
	// XXX: does this work correctly on string literals in all locales?
	mbstowcs(wcs, str, len);
	return wcs;
}

wchar_t* 
gu_vasprintf(const char* fmt, va_list args, GuPool* pool)
{
	GuPool* tmp_pool = gu_pool_new();
	wchar_t* wfmt = gu_str_wcs(fmt, tmp_pool);
	wchar_t* wcs = gu_vaswprintf(wfmt, args, pool);
	gu_pool_free(tmp_pool);
	return wcs;
}

wchar_t* 
gu_asprintf(GuPool* pool, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	wchar_t* wcs = gu_vasprintf(fmt, args, pool);
	va_end(args);
	return wcs;
}
