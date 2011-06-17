#include <gu/fun.h>

static bool
gu_int_eq_fn(GuEqFn* self, const void* a, const void* b)
{
	(void) self;
	return *(const int*) a == *(const int*) b;
}

GuEqFn gu_int_eq = { gu_int_eq_fn };

static size_t
gu_int_hash_fn(GuHashFn* self, const void* p)
{
	(void) self;
	return (size_t) *(const int*) p;
}

GuHashFn gu_int_hash = { gu_int_hash_fn };

