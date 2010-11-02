
#include <glib.h>

#include <gu/mem.h>

typedef GHashTable GuMap;

GuMap*
gu_map_new(GuMemPool* pool, GHashFunc hash, GEqualFunc equal);

static inline gpointer
gu_map_get(GuMap* map, gpointer key)
{
	return g_hash_table_lookup(map, key);
}

static inline void
gu_map_set(GuMap* map, gpointer key, gpointer value)
{
	g_hash_table_insert(map, key, value);
}


typedef GuMap GuIntMap;

static inline GuIntMap*
gu_intmap_new(GuMemPool* pool)
{
	return gu_map_new(pool, NULL, NULL);
}

static inline gpointer
gu_intmap_get(GuIntMap* m, gint key)
{
	return gu_map_get(m, GINT_TO_POINTER(key));
}

static inline void
gu_intmap_set(GuIntMap* m, gint key, gpointer value)
{
	return gu_map_set(m, GINT_TO_POINTER(key), value);
}
