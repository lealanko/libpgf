#include <gu/defs.h>
#include <gu/mem.h>
#include <gu/map.h>
#include <gu/assert.h>
#include <gu/prime.h>


typedef struct GuMapField GuMapField;

struct GuMapField {
	size_t size;
	uint8_t* data;
};

struct GuMap {
	GuPool* pool;
	GuHashFn* hash_fn;
	GuEqFn* eq_fn;

	int n_occupied;
	int n_entries;

	GuMapField keys;
	GuMapField values;

	void* empty_value;
};

static const uint64_t gu_map_empty_key = UINT64_C(0xdeadc0defee15bad);

static size_t
gu_map_field_size(GuMapField* fld) {
	return fld->size ? fld->size : sizeof(void*);
}


static void* 
gu_map_field_get(GuMapField* fld, size_t idx) 
{
	return (fld->size 
		? &fld->data[fld->size * idx] 
		: ((void**)fld->data)[idx]);
}

static void
gu_map_field_set(GuMapField* fld, size_t idx, void* val) 
{
	if (fld->size) {
		memcpy(&fld->data[fld->size * idx], val, fld->size);
	} else {
		((void**)fld->data)[idx] = val;
	}
}

static void
gu_map_finalize(GuFn* fnp)
{
	GuClo1* clo = gu_container(fnp, GuClo1, fn);
	GuMap* map = clo->env1;
	gu_mem_buf_free(map->keys.data);
	gu_mem_buf_free(map->values.data);
}

static bool
gu_map_value_is_empty(GuMap* map, void* value)
{
	if (map->values.size) {
		return (memcmp(value, map->empty_value, map->values.size) == 0);
	} else { 
		return value == map->empty_value;
	}
}

static bool
gu_map_lookup(GuMap* map, const void* key, size_t* idx_out)
{
	GuHashFn* hash_fn = map->hash_fn;
	GuEqFn* eq_fn = map->eq_fn;
	size_t hash = 0;
	if (hash_fn) {
		hash = gu_apply(hash_fn, key);
	} else {
		gu_assert(!map->keys.size);
		hash = (uintptr_t) key;
	}
	size_t n = map->n_entries;
	size_t offset = (hash % (n - 1)) + 1;
	size_t idx = 0;
	size_t key_size = map->keys.size;
	bool present = false;
	if (!eq_fn) {
		// fastpath for by-identity maps
		gu_assert(!key_size);
		for (idx = hash % n; ; idx = (idx + offset) % n) {
			const void* entry_key = 
				((const void**)map->keys.data)[idx];
			if (key == entry_key) {
				present = true;
				break;
			} else if (entry_key == &gu_map_empty_key) {
				break;
			}
		}
	} else {
		for (idx = hash % n; ; idx = (idx + offset) % n) {
			const void* entry_key = 
				gu_map_field_get(&map->keys, idx);
			if (gu_apply(eq_fn, key, entry_key)) {
				present = true;
				break;
			} 
			if (!key_size) {
				if (entry_key == &gu_map_empty_key) {
					break;
				}
			} else if (memcmp(entry_key, 
					  &gu_map_empty_key, 
					  GU_MIN(key_size, 
						 sizeof(gu_map_empty_key)))
				   == 0) {
				// potentially empty, verify from value
				void* entry_value = 
					gu_map_field_get(&map->values, idx);
				if (gu_map_value_is_empty(map, entry_value)) {
					break;
				}
			}
		}
	}
	if (idx_out) {
		*idx_out = idx;
	}
	return present;
}
	

static void
gu_map_resize(GuMap* map)
{
	GuMapField old_keys = map->keys;
	GuMapField old_values = map->values;
	int old_n_entries = map->n_entries;

	int req_entries = (map->n_occupied + 1) * 5 / 4;

	size_t key_size = gu_map_field_size(&map->keys);
	size_t key_alloc = 0;
	map->keys.data = gu_mem_buf_alloc(req_entries * key_size, &key_alloc);

	size_t value_size = gu_map_field_size(&map->values);
	size_t value_alloc = 0;
	map->values.data = 
		gu_mem_buf_alloc(req_entries * value_size, &value_alloc);
	
	map->n_entries = gu_prime_inf(GU_MIN(key_alloc / key_size,
					     value_alloc / value_size));
	gu_assert(map->n_entries > map->n_occupied);
	
	map->n_occupied = 0;

	for (int i = 0; i < map->n_entries; i++) {
		gu_map_field_set(&map->values, i, map->empty_value);
		if (map->keys.size) {
			void* key = gu_map_field_get(&map->keys, i);
			memcpy(key, &gu_map_empty_key, 
			       GU_MIN(map->keys.size, 
				      sizeof(gu_map_empty_key)));
		} else {
			gu_map_field_set(&map->keys, i, 
					 (void*) &gu_map_empty_key);
		}
	}

	for (int i = 0; i < old_n_entries; i++) {
		void* value = gu_map_field_get(&old_values, i);
		if (!gu_map_value_is_empty(map, value)) {
			const void* key = gu_map_field_get(&old_keys, i);
			gu_map_set(map, key, value);
		}
	}
	
	gu_mem_buf_free(old_keys.data);
	gu_mem_buf_free(old_values.data);
}


static void 
gu_map_maybe_resize(GuMap* map)
{
	if (map->n_entries < map->n_occupied + (map->n_occupied / 16) + 1) {
		gu_map_resize(map);
	}
}

bool
gu_map_have(GuMap* map, const void* key)
{
	return gu_map_lookup(map, key, NULL);
}

void*
gu_map_get(GuMap* map, const void* key)
{
	size_t idx;
	gu_map_lookup(map, key, &idx);
	return gu_map_field_get(&map->values, idx);
}

void
gu_map_set(GuMap* map, const void* key, void* value)
{
	size_t idx;
	bool present = gu_map_lookup(map, key, &idx);
	gu_map_field_set(&map->values, idx, value);
	if (!present) {
		gu_map_field_set(&map->keys, idx, (void*) key);
		map->n_occupied++;
		gu_map_maybe_resize(map);
	}
}

void
gu_map_iter(GuMap* map, GuMapIterFn* fn)
{
	for (int i = 0; i < map->n_entries; i++) {
		void* value = gu_map_field_get(&map->values, i);
		if (!gu_map_value_is_empty(map, value)) {
			const void* key = gu_map_field_get(&map->keys, i);
			gu_apply(fn, key, value);
		}
	}
}

GuMap*
gu_map_new_full(GuPool* pool, GuHashFn* hash_fn, GuEqFn* eq_fn, 
		size_t key_size, size_t value_size, void* empty_value)
{
	GuMap* map = gu_new_s(
		pool, GuMap,
		.pool = pool,
		.hash_fn = hash_fn,
		.eq_fn = eq_fn,
		.n_occupied = 0,
		.n_entries = 0);
	map->keys.data = NULL;
	map->keys.size = key_size;
	map->values.data = NULL;
	map->values.size = value_size;
	if (value_size) {
		map->empty_value = 
			gu_malloc_init(pool, value_size, empty_value);
	} else {
		map->empty_value = empty_value;
	}

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


