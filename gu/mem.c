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

#include <gu/mem.h>
#include <gu/fun.h>
#include <gu/bits.h>
#include <gu/assert.h>
#include <gu/log.h>
#include <string.h>
#include <stdlib.h>

static const size_t
// Maximum request size for a chunk. The actual maximum chunk size
// may be somewhat larger.
gu_mem_chunk_max_size = 1024 * sizeof(void*),

// number of bytes to allocate in the pool when it is created
	gu_mem_pool_initial_size = 16 * sizeof(void*),

// Pool allocations larger than this will get their own chunk if
// there's no room in the current one. Allocations smaller than this may trigger
// the creation of a new chunk, in which case the remaining space in
// the current chunk is left unused (internal fragmentation).
	gu_mem_max_shared_alloc = 64 * sizeof(void*),
	
// Should not be smaller than the granularity for malloc
	gu_mem_unit_size = 2 * sizeof(void*),

/* Malloc tuning: the additional memory used by malloc next to the
   allocated object */
	gu_malloc_overhead = sizeof(size_t);




typedef union {
	char c;
	short s;
	int i;
	long l;
	long long ll;
	intmax_t im;
	float f;
	double d;
	long double ld;
	void* p;
	void (*fp)();
} GuMaxAlign;


static void*
gu_mem_realloc(void* p, size_t size)
{
	void* buf = realloc(p, size);
	if (size != 0 && buf == NULL) {
		gu_fatal("Memory allocation failed");
	}
	gu_debug("%p %zu -> %p", p, size, buf); // strictly illegal
	return buf;
}

static void*
gu_mem_alloc(size_t size)
{
	void* buf = malloc(size);
	if (buf == NULL) {
		gu_fatal("Memory allocation failed");
	}
	gu_debug("%zu -> %p", size, buf);
	return buf;
}

static void
gu_mem_free(void* p)
{
	gu_debug("%p", p);
	free(p);
}

static size_t
gu_mem_padovan(size_t min)
{
	// This could in principle be done faster with Q-matrices for
	// Padovan numbers, but not really worth for our commonly
	// small numbers.
	if (min <= 5) {
		return min;
	}
	size_t a = 7, b = 9, c = 12;
	while (min > a) {
		if (b < a) {
			// overflow
			return min;
		}
		size_t tmp = a + b;
		a = b;
		b = c;
		c = tmp;
	}
	return a;
}

void*
gu_mem_buf_realloc(void* old_buf, size_t min_size, size_t* real_size_out)
{
	size_t min_blocks = ((min_size + gu_malloc_overhead - 1) /
			     gu_mem_unit_size) + 1;
	size_t blocks = gu_mem_padovan(min_blocks);
	size_t size = blocks * gu_mem_unit_size - gu_malloc_overhead;
	void* buf = gu_mem_realloc(old_buf, size);
	*real_size_out = size;
	return buf;
}
void*
gu_mem_buf_alloc(size_t min_size, size_t* real_size_out)
{
	return gu_mem_buf_realloc(NULL, min_size, real_size_out);
}

void
gu_mem_buf_free(void* buf)
{
	gu_mem_free(buf);
}


typedef struct GuMemChunk GuMemChunk;

struct GuMemChunk {
	GuMemChunk* next;
	uint8_t data[];
};

typedef struct GuFinalizerNode GuFinalizerNode;

struct GuFinalizerNode {
	GuFinalizerNode* next;
	GuFinalizer* fin;
};

struct GuPool {
	uint8_t* curr_buf; // actually GuMemChunk*
	GuMemChunk* chunks;
	GuFinalizerNode* finalizers;
	uint16_t left_edge;
	uint16_t right_edge;
	uint16_t curr_size;
	uint8_t init_buf[];
};

GuPool* 
gu_pool_new(void)
{
	size_t sz = GU_FLEX_SIZE(GuPool, init_buf, gu_mem_pool_initial_size);
	GuPool* pool = gu_mem_buf_alloc(sz, &sz);
	pool->curr_size = sz;
	pool->curr_buf = (uint8_t*) pool;
	pool->chunks = NULL;
	pool->finalizers = NULL;
	pool->left_edge = offsetof(GuPool, init_buf);
	pool->right_edge = sz;
	return pool;
}

static void
gu_pool_expand(GuPool* pool, size_t req)
{
	size_t real_req = GU_MAX(req, GU_MIN(((size_t)pool->curr_size) + 1,
					     gu_mem_chunk_max_size));
	gu_assert(real_req >= sizeof(GuMemChunk));
	size_t size = 0;
	GuMemChunk* chunk = gu_mem_buf_alloc(real_req, &size);
	chunk->next = pool->chunks;
	pool->chunks = chunk;
	pool->curr_buf = (uint8_t*) chunk;
	pool->left_edge = offsetof(GuMemChunk, data);
	pool->right_edge = pool->curr_size = size;
	// size should always fit in uint16_t
	gu_assert((size_t) pool->right_edge == size); 
}

static size_t
gu_mem_advance(size_t old_pos, size_t pre_align, size_t pre_size,
	       size_t align, size_t size)
{
	size_t p = gu_align_forward(old_pos, pre_align);
	p += pre_size;
	p = gu_align_forward(p, align);
	p += size;
	return p;
}

static void*
gu_pool_malloc_aligned(GuPool* pool, size_t pre_align, size_t pre_size,
		       size_t align, size_t size) 
{
	gu_require(pre_align > 0 && align > 0);
	gu_require(size <= gu_mem_max_shared_alloc);
	size_t pos = gu_mem_advance(pool->left_edge, pre_align, pre_size,
				    align, size);
	if (pos > (size_t) pool->right_edge) {
		pos = gu_mem_advance(offsetof(GuMemChunk, data),
				     pre_align, pre_size, align, size);
		gu_pool_expand(pool, pos);
		gu_assert(pos <= pool->right_edge);
	} 
	pool->left_edge = pos;
	return &pool->curr_buf[pos - size];

}

static size_t
gu_pool_avail(GuPool* pool)
{
	return (size_t) pool->right_edge - (size_t) pool->left_edge;
}

void*
gu_pool_malloc_unaligned(GuPool* pool, size_t size)
{
	if (size > gu_pool_avail(pool)) {
		gu_pool_expand(pool, offsetof(GuMemChunk, data) + size);
		gu_assert(size <= gu_pool_avail(pool));
	}
	pool->right_edge -= size;
	return &pool->curr_buf[pool->right_edge];
}

void*
gu_malloc_prefixed(GuPool* pool, size_t pre_align, size_t pre_size,
		   size_t align, size_t size)
{
	gu_require(gu_aligned(pre_size, pre_align));
	gu_require(gu_aligned(size, align));
	size_t full_size = gu_mem_advance(offsetof(GuMemChunk, data),
					  pre_align, pre_size, align, size);
	if (full_size > gu_mem_max_shared_alloc) {
		GuMemChunk* chunk = gu_mem_alloc(full_size);
		chunk->next = pool->chunks;
		pool->chunks = chunk;
		return &chunk->data[full_size - size
				    - offsetof(GuMemChunk, data)];
	} else if (pre_align == 1 && align == 1) {
		uint8_t* buf = gu_pool_malloc_unaligned(pool, pre_size + size);
		return &buf[pre_size];
	} else {
		return gu_pool_malloc_aligned(pool, pre_align, pre_size,
					      align, size);
	}
}

void*
gu_malloc_aligned(GuPool* pool, size_t size, size_t align)
{
	gu_enter("-> %p %zu %zu", pool, size, align);
	if (align == 0) {
		align = GU_MIN(size, gu_alignof(GuMaxAlign));
	}
	void* ret = gu_malloc_prefixed(pool, 1, 0, align, size);
	gu_exit("<- %p", ret);
	return ret;
}


void 
gu_pool_finally(GuPool* pool, GuFinalizer* finalizer)
{
	GuFinalizerNode* node = gu_new(pool, GuFinalizerNode);
	node->next = pool->finalizers;
	node->fin = finalizer;
	pool->finalizers = node;
}

void
gu_pool_free(GuPool* pool)
{
	GuFinalizerNode* node = pool->finalizers;
	while (node) {
		GuFinalizerNode* next = node->next;
		node->fin->fn(node->fin);
		node = next;
	}
	GuMemChunk* chunk = pool->chunks;
	while (chunk) {
		GuMemChunk* next = chunk->next;
		gu_mem_buf_free(chunk);
		chunk = next;
	}
	gu_mem_buf_free(pool);
}


extern inline void* gu_malloc(GuPool* pool, size_t size);

extern inline void* 
gu_malloc_init_aligned(GuPool* pool, size_t size, size_t alignment, 
		       const void* init);

