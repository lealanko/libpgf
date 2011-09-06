#include <gu/str.h>
#include <string.h>
#include <stdio.h>

char* 
gu_str_alloc(int size, GuPool* pool)
{
	char* str = gu_malloc_aligned(pool, size + 1, 1);
	str[size] = '\0';
	return str;
}

char*
gu_strdup(const char* cstr, GuPool* pool)
{
	int len = strlen(cstr);
	char* str = gu_str_alloc(len, pool);
	memcpy(str, cstr, len);
	return str;
}

char* 
gu_vasprintf(const char* fmt, va_list args, GuPool* pool)
{
	va_list args2;
	va_copy(args2, args);
	int len = vsnprintf(NULL, 0, fmt, args2);
	va_end(args2);
	char* str = gu_str_alloc(len, pool);
	vsnprintf(str, len + 1, fmt, args);
	return str;
}

char* 
gu_asprintf(GuPool* pool, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char* str = gu_vasprintf(fmt, args, pool);
	va_end(args);
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
