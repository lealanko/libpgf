#include <gu/map.h>
#include <gu/str.h>
#include <gu/string.h>
#include <gu/assert.h>
#include <stdio.h>

void test_strs(int n)
{
	GuPool* pool = gu_make_pool();
	GuMap* ht = gu_new_map(GuStr, gu_str_hasher,
			       int, NULL, pool);
	for (int i = 0; i < n; i++) {
		char* str = gu_asprintf(pool, "%d", i);
		gu_map_put(ht, &str, int, i * i);
	}
	for (int i = 0; i < n; i++) {
		char* str = gu_asprintf(pool, "%d", i);
		int ip = gu_map_get(ht, &str, int);
		gu_assert(ip == i * i);
	}
	gu_pool_free(pool);
}

void test_strings(int n)
{
	GuPool* pool = gu_make_pool();
	GuMap* ht = gu_new_map(GuString, gu_string_hasher,
			       int, NULL, pool);
	for (int i = 0; i < n; i++) {
		GuString s = gu_format_string(pool, "%d", i);
		gu_map_put(ht, &s, int, i * i);
	}
	for (int i = 0; i < n; i++) {
		GuString s = gu_format_string(pool, "%d", i);
		int ip = gu_map_get(ht, &s, int);
		gu_assert(ip == i * i);
	}
	gu_pool_free(pool);
}

void test_ints(int n)
{
	GuPool* pool = gu_make_pool();
	GuMap* ht = gu_new_map(int, gu_int_hasher,
			       int, NULL, pool);
	for (int i = 0; i < n; i++) {
		gu_map_put(ht, &i, int, i * i);
	}
	for (int i = 1; i < n; i++) {
		int ip = gu_map_get(ht, &i, int);
		gu_assert(ip == i * i);
	}
	gu_pool_free(pool);
}

int main(void)
{
	test_ints(10000);
	test_strs(10000);
	test_strings(10000);
}

