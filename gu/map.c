
#include "map.h"

static void
gu_map_free_cb(void* ht) 
{
	g_hash_table_destroy(ht);
}

GuMap*
gu_map_new(GuPool* pool, GHashFunc hash, GEqualFunc equal)
{
	GHashTable* ht = g_hash_table_new(hash, equal);
	gu_pool_finally(pool, gu_map_free_cb, ht);
	return ht;
}
