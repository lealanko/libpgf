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
#include "fun.h"
#include <string.h>

typedef union {
	long long ll;
	long double ld;
	void* p;
	void (*f)();
} GuMaxAligned;

enum {
	GU_MAX_ALIGNMENT = gu_alignof(GuMaxAligned)
};


typedef struct GuMemFinalizerNode GuMemFinalizerNode;	

struct GuMemFinalizerNode {
	GuFn0* finalizer;
	GuMemFinalizerNode* next;
};

// TODO: include some storage in the buffer object to make temporary
// pools efficient

struct GuPool {
	uint8_t* cur;
	uint8_t* end;
	GPtrArray* chunks;
	GuMemFinalizerNode* finalizers;
};

typedef struct Finalizer Finalizer;

struct Finalizer {
	GDestroyNotify func;
	void* p;
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
	pool->finalizers = NULL;
	pool->cur = g_malloc(CHUNK_SIZE);
	pool->end = &pool->cur[CHUNK_SIZE];
	g_ptr_array_add(pool->chunks, pool->cur);
	return pool;
}


G_STATIC_ASSERT((CHUNK_SIZE >= MAX_CHUNKABLE_SIZE));

// We assume that all alignments are powers of two
size_t
gu_mem_alignment(size_t size)
{
	size_t align = 2;
	while (size % align == 0) {
		align = align * 2;
	}
	return MIN(GU_MAX_ALIGNMENT, align / 2);
}



void*
gu_malloc_aligned(GuPool* pool, size_t size, size_t alignment)
{
	if (size > MAX_CHUNKABLE_SIZE) {
		void* p = g_malloc(size);
		g_ptr_array_add(pool->chunks, p);
		return p;
	}
	if (alignment == 0) {
		alignment = gu_mem_alignment(size);
	}
	uintptr_t cur = (uintptr_t)pool->cur;
	cur = ((cur + alignment - 1) / alignment) * alignment;
	uint8_t* p = (void*) cur;
	if (pool->end - p < (ptrdiff_t)size) {
		p = g_malloc(CHUNK_SIZE);
		pool->end = &p[CHUNK_SIZE];
		g_ptr_array_add(pool->chunks, p);
	}
	pool->cur = p + size;
	return p;
}

void 
gu_pool_finally(GuPool* pool, GuFn0* finalize)
{
	GuMemFinalizerNode* node = gu_new(pool, GuMemFinalizerNode);
	node->finalizer = finalize;
	node->next = pool->finalizers;
	pool->finalizers = node;
}

void
gu_pool_free(GuPool* pool)
{
	GuMemFinalizerNode* node = pool->finalizers;
	while (node != NULL) {
		(*node->finalizer)(node->finalizer);
		node = node->next;
	}
	g_ptr_array_free(pool->chunks, TRUE);
	g_slice_free(GuPool, pool);
}


extern inline void* gu_malloc(GuPool* pool, size_t size);

extern inline void* 
gu_malloc_init_aligned(GuPool* pool, size_t size, size_t alignment, 
		       const void* init);

