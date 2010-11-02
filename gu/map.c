
#include "map.h"

static void
gu_map_free_cb(gpointer ht) 
{
	g_hash_table_destroy(ht);
}

GuMap*
gu_map_new(GuMemPool* pool, GHashFunc hash, GEqualFunc equal)
{
	GHashTable* ht = g_hash_table_new(hash, equal);
	gu_mem_pool_register_finalizer(pool, gu_map_free_cb, ht);
	return ht;
}
