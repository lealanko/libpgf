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

	


struct GuAllocator {
	gpointer (*alloc)(GuAllocator* ator, gsize size, gsize alignment);
};

gpointer gu_malloc_aligned(GuAllocator* ator, gsize size, gsize alignment)
{
	if (ator == NULL) {
		ator = gu_malloc_allocator();
	}
	return ator->alloc(ator, size, alignment);
}

static gpointer
gu_malloc_alloc_fn(G_GNUC_UNUSED GuAllocator* ator, gsize size, 
		   G_GNUC_UNUSED gsize alignment)
{
	return g_malloc(size);
}

static GuAllocator gu_malloc_ator = {
	.alloc = gu_malloc_alloc_fn
};

GuAllocator* gu_malloc_allocator(void)
{
	return &gu_malloc_ator;
}

static gpointer
gu_slice_alloc_fn(G_GNUC_UNUSED GuAllocator* ator, gsize size, 
		  G_GNUC_UNUSED gsize alignment)
{
	return g_slice_alloc(size);
}

static GuAllocator gu_slice_ator = {
	.alloc = gu_slice_alloc_fn
};

GuAllocator* gu_slice_allocator(void)
{
	return &gu_slice_ator;
}

struct GuMemPool {
	guint8* cur;
	guint8* end;
	GPtrArray* chunks;
	GuAllocator ator;
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


static gpointer
gu_mem_pool_alloc_fn(GuAllocator* ator, gsize size, gsize alignment);

GuMemPool*
gu_mem_pool_new(void) 
{
	GuMemPool* pool = g_slice_new(GuMemPool);
	pool->cur = NULL;
	pool->end = NULL;
	pool->ator.alloc = gu_mem_pool_alloc_fn;
	pool->chunks = g_ptr_array_new_with_free_func(g_free);
	pool->finalizers = g_hash_table_new(NULL, NULL);
	pool->cur = g_malloc(CHUNK_SIZE);
	pool->end = &pool->cur[CHUNK_SIZE];
	g_ptr_array_add(pool->chunks, pool->cur);
	return pool;
}

GuAllocator*
gu_mem_pool_allocator(GuMemPool* pool)
{
	return &pool->ator;
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



static gpointer
gu_mem_pool_alloc(GuMemPool* pool, gsize size, gsize alignment)
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

static gpointer
gu_mem_pool_alloc_fn(GuAllocator* ator, gsize size, gsize alignment)
{
	GuMemPool* pool = GU_CONTAINER_P(ator, GuMemPool, ator);
	return gu_mem_pool_alloc(pool, size, alignment);
}

void 
gu_mem_pool_register_finalizer(GuMemPool* pool, GDestroyNotify func, gpointer p)
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
gu_mem_pool_run_finalizer(gpointer key, gpointer value, 
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
gu_mem_pool_free(GuMemPool* pool)
{
	g_hash_table_foreach(pool->finalizers, gu_mem_pool_run_finalizer, NULL);
	g_hash_table_destroy(pool->finalizers);
	g_ptr_array_free(pool->chunks, TRUE);
	g_slice_free(GuMemPool, pool);
}

extern inline gpointer gu_malloc(GuAllocator* ator, gsize size);
