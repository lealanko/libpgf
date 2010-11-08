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

#include "mem.h"
#include "macros.h"

#include <stdint.h>



typedef union {
	long long ll;
	long double ld;
	void* p;
	void (*f)();
} GuMaxAligned;

enum {
	GU_MAX_ALIGNMENT = gu_alignof(GuMaxAligned)
};

	




struct GuPool {
	guint8* cur;
	guint8* end;
	GPtrArray* chunks;
	GHashTable* finalizers;
};

typedef struct Finalizer Finalizer;

struct Finalizer {
	GDestroyNotify func;
	gpointer p;
};

enum {
	CHUNK_SIZE = 8192,
	MAX_CHUNKABLE_SIZE = 128
};


GuPool*
gu_pool_new(void) 
{
	GuPool* pool = g_slice_new(GuPool);
	pool->cur = NULL;
	pool->end = NULL;
	pool->chunks = g_ptr_array_new_with_free_func(g_free);
	pool->finalizers = g_hash_table_new(NULL, NULL);
	pool->cur = g_malloc(CHUNK_SIZE);
	pool->end = &pool->cur[CHUNK_SIZE];
	g_ptr_array_add(pool->chunks, pool->cur);
	return pool;
}


G_STATIC_ASSERT((CHUNK_SIZE >= MAX_CHUNKABLE_SIZE));

// We assume that all alignments are powers of two
gsize
gu_mem_alignment(gsize size)
{
	gsize align = 2;
	while (size % align == 0) {
		align = align * 2;
	}
	return MIN(GU_MAX_ALIGNMENT, align / 2);
}



gpointer
gu_malloc_aligned(GuPool* pool, gsize size, gsize alignment)
{
	if (size > MAX_CHUNKABLE_SIZE) {
		gpointer p = g_malloc(size);
		g_ptr_array_add(pool->chunks, p);
		return p;
	}
	if (alignment == 0) {
		alignment = gu_mem_alignment(size);
	}
	gintptr cur = (gintptr)pool->cur;
	cur = ((cur + alignment - 1) / alignment) * alignment;
	guint8* p = (gpointer) cur;
	if (pool->end - p < (goffset)size) {
		p = g_malloc(CHUNK_SIZE);
		pool->end = &p[CHUNK_SIZE];
		g_ptr_array_add(pool->chunks, p);
	}
	pool->cur = p + size;
	return p;
}

void 
gu_pool_finally(GuPool* pool, GDestroyNotify func, gpointer p)
{
	gpointer key = (gpointer)func; // Not strictly portable
	GPtrArray* arr = g_hash_table_lookup(pool->finalizers, key);
	if (arr == NULL) {
		arr = g_ptr_array_new();
		g_hash_table_insert(pool->finalizers, key, arr);
	}
	g_ptr_array_add(arr, p);
}

static void
gu_pool_run_finalizer(gpointer key, gpointer value, 
			  G_GNUC_UNUSED gpointer user_data)
{
	GDestroyNotify destroy = (GDestroyNotify) key;
	GPtrArray* arr = value;
	gint len = arr->len;
	for (gint i = 0; i < len; i++) {
		destroy(arr->pdata[i]);
	}
	g_ptr_array_free(arr, TRUE);
}

void
gu_pool_free(GuPool* pool)
{
	g_hash_table_foreach(pool->finalizers, gu_pool_run_finalizer, NULL);
	g_hash_table_destroy(pool->finalizers);
	g_ptr_array_free(pool->chunks, TRUE);
	g_slice_free(GuPool, pool);
}


extern inline gpointer gu_malloc(GuPool* pool, gsize size);

