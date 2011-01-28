
#include "map.h"

static void
gu_map_free_fn(GuFn* fnp) 
{
	GuClo1* clo = (GuClo1*) fnp;
	g_hash_table_destroy(clo->env1);
}


GuMap*
gu_map_new(GuPool* pool, GHashFunc hash, GEqualFunc equal)
{
	GHashTable* ht = g_hash_table_new(hash, equal);
	GuClo1* clo = gu_new_s(pool, GuClo1, gu_map_free_fn, ht);
	gu_pool_finally(pool, &clo->fn);
	return ht;
}



static void
gu_map_iter_cb(void* key, void* value, void* user_data)
{
	GuFn2* fn = user_data;
	gu_apply2(fn, key, value);
}

void
gu_map_iter(GuMap* map, GuFn2* fn)
{
	g_hash_table_foreach(map, gu_map_iter_cb, fn);
}

GU_DEFINE_TYPE(GuMapDirectInt, abstract, _);

GU_DEFINE_KIND(GuMap, abstract);
GU_DEFINE_KIND(GuIntMap, GuMap);
GU_DEFINE_KIND(GuStringMap, GuMap);
