#include <gu/defs.h>
#include <gu/mem.h>
#include <gu/map.h>
#include <gu/prime.h>

typedef struct GuMapEntry GuMapEntry;

struct GuMap {
	GuPool* pool;
	GuHashFn* hash_fn;
	GuEqFn* eq_fn;

	int n_occupied;
	int n_entries;
	GuMapEntry* entries;
};

struct GuMapEntry {
	const void* key;
	void* value;
};


static void
gu_map_finalize(GuFn* fnp)
{
	GuClo1* clo = gu_container(fnp, GuClo1, fn);
	GuMap* map = clo->env1;
	gu_mem_buf_free(map->entries);
}


static GuMapEntry*
gu_map_try_entry(GuMap* map, const void* key, size_t idx)
{
	GuMapEntry* entry = &map->entries[idx];
	if (entry->value == NULL) {
		return entry;
	}
	bool match = false;
	if (map->eq_fn != NULL) {
		match = map->eq_fn->fn(map->eq_fn, key, entry->key);
	} else {
		match = (key == entry->key);
	}
	if (match) {
		return entry;
	}
	return NULL;
}

static GuMapEntry* 
gu_map_lookup_entry(GuMap* map, const void* key)
{
	GuHashFn* fn = map->hash_fn;
	size_t hash = fn == NULL ? (uintptr_t) key : fn->fn(fn, key);
	size_t n = map->n_entries;
	size_t offset = (hash % (n - 1)) + 1;
	GuMapEntry* entry = NULL;
	for (size_t idx = hash % n; entry == NULL; idx = (idx + offset) % n) {
		entry = gu_map_try_entry(map, key, idx);
	}
	return entry;
}
	

static void
gu_map_resize(GuMap* map)
{
	GuMapEntry* old_entries = map->entries;
	int old_n_entries = map->n_entries;
	size_t new_size = 0;
	size_t new_size_req = sizeof(GuMapEntry) * (map->n_occupied + 1) * 5 / 4;
	map->entries = gu_mem_buf_alloc(new_size_req, &new_size);
	gu_assert(new_size != 0); // XXX: do properly
	
	map->n_entries = gu_prime_inf(new_size / sizeof(GuMapEntry));
	map->n_occupied = 0;

	for (int i = 0; i < map->n_entries; i++) {
		GuMapEntry* entry = &map->entries[i];
		entry->key = 0;
		entry->value = NULL;
	}

	for (int i = 0; i < old_n_entries; i++) {
		GuMapEntry* entry = &old_entries[i];
		if (entry->value != NULL) {
			gu_map_set(map, entry->key, entry->value);
		}
	}
	
	gu_mem_buf_free(old_entries);
}


static void 
gu_map_maybe_resize(GuMap* map)
{
	if (map->n_entries < map->n_occupied + (map->n_occupied / 16) + 1) {
		gu_map_resize(map);
	}
}

void*
gu_map_get(GuMap* map, const void* key)
{
	GuMapEntry* entry = gu_map_lookup_entry(map, key);
	return entry->value;
}

void
gu_map_set(GuMap* map, const void* key, void* value)
{
	GuMapEntry* entry = gu_map_lookup_entry(map, key);
	bool was_empty = entry->value == NULL;
	entry->key = key;
	entry->value = value;
	if (was_empty) {
		map->n_occupied++;
		gu_map_maybe_resize(map);
	}
}

void
gu_map_iter(GuMap* map, GuMapIterFn* fn)
{
	for (int i = 0; i < map->n_entries; i++) {
		GuMapEntry* entry = &map->entries[i];
		if (entry->value != NULL) {
			fn->fn(fn, entry->key, entry->value);
		}
	}
}

GuMap*
gu_map_new(GuPool* pool, GuHashFn* hash_fn, GuEqFn* eq_fn)
{
	GuMap* map = gu_new_s(
		pool, GuMap,
		.pool = pool,
		.hash_fn = hash_fn,
		.eq_fn = eq_fn,
		.n_occupied = 0,
		.n_entries = 0,
		.entries = NULL);
	GuClo1* clo = gu_new_s(
		pool, GuClo1,
		.fn = gu_map_finalize,
		.env1 = map);
	gu_pool_finally(pool, &clo->fn);
	gu_map_resize(map);
	return map;
}

GU_DEFINE_KIND(GuMap, abstract);
GU_DEFINE_KIND(GuStringMap, GuMap);
GU_DEFINE_KIND(GuIntMap, GuMap);


GuStringMap*
gu_stringmap_new(GuPool* pool)
{
	return gu_map_new(pool, &gu_string_hash, &gu_string_eq);
}

GuIntMap*
gu_intmap_new(GuPool* pool)
{
	return gu_map_new(pool, &gu_int_hash, &gu_int_eq);
}

void
gu_intmap_set(GuIntMap* m, int key, void* value)
{
	void* old = gu_intmap_get(m, key);
	int* kp = NULL;
	if (old == NULL) {
		kp = gu_new_s(m->pool, int, key);
	} else {
		kp = &key;
	}
	gu_map_set(m, kp, value);
}
