#include <gu/map.h>
#include <gu/string.h>
#include <stdio.h>

void test_strings(int n)
{
	GuPool* pool = gu_pool_new();
	GuMap* ht = gu_map_new(pool, &gu_string_hash, &gu_string_eq);
	for (int i = 0; i < n; i++) {
		GuString* str = gu_string_format(pool, "%d", i);
		int* ip = gu_new_s(pool, int, i * i);
		gu_map_set(ht, str, ip);
	}
	for (int i = 0; i < n; i++) {
		GuString* str = gu_string_format(pool, "%d", i);
		int* ip = gu_map_get(ht, str);
		gu_assert(*ip == i * i);
	}
	gu_pool_free(pool);
}

#if 0
void test_ints(int n)
{
	GuPool* pool = gu_pool_new();
	GuMap* ht = gu_map_new(pool, NULL, NULL,
					    (uintptr_t) -1);
	for (int i = 0; i < n; i++) {
		gu_map_set(ht, (uintptr_t) i, (uintptr_t) i * i);
	}
	for (int i = 0; i < n; i++) {
		uintptr_t u = gu_map_get(ht, (uintptr_t) i);
		gu_assert((int) u == i * i);
	}
	gu_pool_free(pool);
}
#endif

int main(void)
{
	// test_ints(10000);
	test_strings(10000);
}

