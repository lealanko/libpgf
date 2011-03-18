/* 
 * Copyright 2010 University of Helsinki.
 *   
 * This file is part of libgu.
 * 
 * Libgu is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * Libgu is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libgu. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GU_MAP_H_
#define GU_MAP_H_

#include <glib.h>
#include <gu/mem.h>
#include <gu/fun.h>

typedef GHashTable GuMap;

GuMap*
gu_map_new(GuPool* pool, GHashFunc hash, GEqualFunc equal);

static inline void*
gu_map_get(GuMap* map, const void* key)
{
	return g_hash_table_lookup(map, key);
}

static inline void
gu_map_set(GuMap* map, const void* key, void* value)
{
	g_hash_table_insert(map, (void*) key, value);
}

void 
gu_map_iter(GuMap* map, GuFn2* fn);

static inline bool
gu_map_has(GuMap* map, const void* key)
{
	return g_hash_table_lookup_extended(map, key, NULL, NULL);
}


typedef GuMap GuIntMap;

static inline GuIntMap*
gu_intmap_new(GuPool* pool)
{
	return gu_map_new(pool, NULL, NULL);
}

static inline void*
gu_intmap_get(GuIntMap* m, int key)
{
	return gu_map_get(m, GINT_TO_POINTER(key));
}

static inline void
gu_intmap_set(GuIntMap* m, int key, void* value)
{
	gu_map_set(m, GINT_TO_POINTER(key), value);
}

static inline bool
gu_intmap_has(GuIntMap* map, int key)
{
	return gu_map_has(map, GINT_TO_POINTER(key));
}

static inline int
gu_map_size(GuMap* map) 
{
	return (int) g_hash_table_size(map);
}



#include <gu/string.h>

typedef GuMap GuStringMap;

static inline GuStringMap*
gu_stringmap_new(GuPool* pool)
{
	return gu_map_new(pool, gu_string_hash, gu_string_equal);
}

static inline void*
gu_stringmap_get(GuStringMap* m, const GuString* key)
{
	return gu_map_get(m, key);
}

static inline void
gu_stringmap_set(GuStringMap* m, const GuString* key, void* value)
{
	gu_map_set(m, key, value);
}




#include <gu/type.h>

extern GU_DECLARE_KIND(GuMap);

typedef const struct GuMapType GuMapType, GuType_GuMap;

struct GuMapType {
	GuType_abstract abstract_base;
	GuType* key_type;
	GuType* value_type;
};

#define GU_TYPE_INIT_GuMap(k_, t_, key_type_, value_type_) {	\
	.abstract_base = GU_TYPE_INIT_abstract(k_, t_, _),	\
	.key_type = key_type_, \
	.value_type = value_type_ \
}	

typedef void GuMapDirectInt;
extern GU_DECLARE_TYPE(GuMapDirectInt, abstract);


extern GU_DECLARE_KIND(GuIntMap);
typedef GuType_GuMap GuType_GuIntMap;

#define GU_TYPE_INIT_GuIntMap(k_, t_, value_type_) \
	GU_TYPE_INIT_GuMap(k_, t_, gu_type(GuMapDirectInt), value_type_)



#include <gu/string.h>

extern GU_DECLARE_KIND(GuStringMap);
typedef GuType_GuMap GuType_GuStringMap;

#define GU_TYPE_INIT_GuStringMap(k_, t_, value_type_) \
	GU_TYPE_INIT_GuMap(k_, t_, gu_type(GuStringP), value_type_)


#endif // GU_MAP_H_
