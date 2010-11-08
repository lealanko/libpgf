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

#include <gu/mem.h>

typedef GHashTable GuMap;

GuMap*
gu_map_new(GuPool* pool, GHashFunc hash, GEqualFunc equal);

static inline void*
gu_map_get(GuMap* map, void* key)
{
	return g_hash_table_lookup(map, key);
}

static inline void
gu_map_set(GuMap* map, void* key, void* value)
{
	g_hash_table_insert(map, key, value);
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
	return gu_map_set(m, GINT_TO_POINTER(key), value);
}

#endif // GU_MAP_H_
