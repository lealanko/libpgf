#include <gu/map.h>
#include <gu/str.h>
#include <gu/assert.h>
#include <stdio.h>

void test_strings(int n)
{
	GuPool* pool = gu_pool_new();
	GuMap* ht = gu_map_new(pool, &gu_str_hash, &gu_str_eq);
	for (int i = 0; i < n; i++) {
		char* str = gu_asprintf(pool, "%d", i);
		int* ip = gu_new_s(pool, int, i * i);
		gu_map_set(ht, str, ip);
	}
	for (int i = 0; i < n; i++) {
		char* str = gu_asprintf(pool, "%d", i);
		int* ip = gu_map_get(ht, str);
		gu_assert(*ip == i * i);
	}
	gu_pool_free(pool);
}

void test_ints(int n)
{
	GuPool* pool = gu_pool_new();
	GuIntMap* ht = gu_intmap_new(pool);
	for (int i = 0; i < n; i++) {
		int* ip = gu_new_s(pool, int, i * i);
		gu_intmap_set(ht, i, ip);
	}
	for (int i = 0; i < n; i++) {
		int* ip = gu_intmap_get(ht, i);
		gu_assert(*ip == i * i);
	}
	gu_pool_free(pool);
}

int main(void)
{
	test_ints(10000);
	test_strings(10000);
}

