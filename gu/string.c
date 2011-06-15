#include <string.h>
#include <limits.h>

#include "string.h"
#include "list.h"


const GuStringEmpty_
gu_string_empty_ = {
	.true_len = 0,
	.ss = {
		.len = 0,
		.data = "\0",
	},
};

// the second element is just for neatness, it's nice that
// gu_string_data always returns a pointer to something sensible

GuString* 
gu_string_new(GuPool* pool, int len)
{
	if (len == 0) {
		return (GuString*)gu_string_empty;
	} else if (len > 0 && len <= UCHAR_MAX) {
		unsigned char* up = gu_malloc_aligned(pool, 1 + len, 1);
		up[0] = len;
		return (GuString*) up;
	} else {
		int* ip = gu_malloc_aligned(pool, sizeof(int) + 1 + len,
					    gu_alignof(int));
		ip[0] = len;
		unsigned char* up = (unsigned char*) &ip[1];
		up[0] = 0;
		return (GuString*) up;
	}
}



extern inline char*
gu_string_data(GuString* str);

extern inline const char*
gu_string_cdata(const GuString* str);

extern inline int
gu_string_length(const GuString* str);

GuString* 
gu_string_new_c(GuPool* pool, const char* cstr)
{
	int len = strlen(cstr);
	GuString* s = gu_string_new(pool, len);
	char* data = gu_string_data(s);
	memcpy(data, cstr, len);
	return s;
}

GuString* 
gu_string_copy(GuPool* pool, const GuString* from)
{
	int len = gu_string_length(from);
	GuString* to = gu_string_new(pool, len);
	const char* from_data = gu_string_cdata(from);
	char* to_data = gu_string_data(to);
	memcpy(to_data, from_data, len);
	return to;
}

static size_t
gu_string_hash_fn(GuHashFn* self, const void* p)
{
	(void) self;
	const GuString* s = p;
	int n = gu_string_length(s);
	const char* cp = gu_string_cdata(s);
	unsigned h = 0;

	for (int i = 0; i < n; i++) {
		h = 101 * h + (unsigned char) cp[i];
	}
	return h;
}

GuHashFn gu_string_hash = { gu_string_hash_fn };

bool
gu_string_equal(const GuString* p1, const GuString* p2)
{
	const GuString* s1 = p1;
	const GuString* s2 = p2;
	int len1 = gu_string_length(s1);
	int len2 = gu_string_length(s2);
	
	if (len1 != len2) {
		return false;
	}
	const char* data1 = gu_string_cdata(s1);
	const char* data2 = gu_string_cdata(s2);
	int cmp = memcmp(data1, data2, len1);
	return (cmp == 0);
}

static bool
gu_string_eq_fn(GuEqFn* self, const void* a, const void* b)
{
	(void) self;
	return gu_string_equal(a, b);
}

GuEqFn gu_string_eq = { gu_string_eq_fn };

#include <stdio.h>

GuString*
gu_string_format_v(GuPool* pool, const char* fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	int len = vsnprintf(NULL, 0, fmt, args2);
	va_end(args2);

	// Alas, upon truncation vsnprintf insists on using the last
	// character for '\0', so we are forced to use a temporary
	// buffer just to accommodate that extra '\0'.
	char buf[len + 1]; 
	vsnprintf(buf, len + 1, fmt, args);

	GuString* str = gu_string_new(pool, len);
	char* data = gu_string_data(str);
	memcpy(data, buf, len);

	return str;
}

GuString*
gu_string_format(GuPool* pool, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	GuString* s = gu_string_format_v(pool, fmt, args);
	va_end(args);
	return s;
}


// type

GU_DEFINE_TYPE(GuString, abstract, _);
GU_DEFINE_TYPE(GuStringP, pointer, gu_type(GuString));

