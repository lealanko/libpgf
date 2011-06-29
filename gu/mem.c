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
#include <string.h>
#include <stdlib.h>

// #define GU_MEM_DEBUG

#ifdef GU_MEM_DEBUG
#include <stdio.h>
#endif // GU_MEM_DEBUG


enum {
	// Must be maximally aligned.
	GU_MEM_UNIT_SIZE = 64,

	GU_MEM_CHUNK_MAX_UNITS = 144,
	// Should be a fibonacci number.

	
	GU_MEM_CHUNK_DEDICATED_THRESHOLD = 256,
	
	// malloc tuning
	// these defaults are tuned for glibc
	
	// The additional memory used by malloc next to the allocated
	// object
	GU_MALLOC_OVERHEAD = sizeof(size_t),

	// Alignment of memory returned by malloc
	GU_MALLOC_ALIGNMENT = GU_MAX(gu_alignof(long double),
				  2 * GU_MALLOC_OVERHEAD),
};

GU_STATIC_ASSERT(GU_MEM_CHUNK_MAX_UNITS <= UINT8_MAX);

// We use lowermost bits of struct pointers for tags,
// hence we must make sure that there is space for them.
GU_STATIC_ASSERT(gu_alignof(GuPool*) >= 4);


typedef struct GuMemNode GuMemNode;

struct GuMemNode {
	uintptr_t u;
};

typedef enum {
	GU_MEM_NODE_CHUNK = 0x0,
	GU_MEM_NODE_FINALIZER = 0x1,
	GU_MEM_NODE_DEDICATED = 0x2,
	GU_MEM_NODE_POOL = 0x3,
} GuMemNodeTag;

static GuMemNodeTag
gu_mem_node_tag(GuMemNode node)
{
	return node.u & 0x3;
}

static void*
gu_mem_node_addr(GuMemNode node)
{
	return (void*) (node.u & ~0x3);
}

static GuMemNode
gu_mem_node(GuMemNodeTag tag, void* addr)
{
	uintptr_t u = (uintptr_t) addr;
	gu_assert((u & 0x3) == 0);
	return (GuMemNode) { u | (uintptr_t) tag };
}


typedef struct GuMemFinalizer GuMemFinalizer;	

struct GuMemFinalizer {
	GuMemNode next;
	GuFn0* finalizer;
};

typedef struct GuMemChunk GuMemChunk;

struct GuMemChunk {
	GuMemNode next;
	uint8_t data[];
};

struct GuPool {
	uint8_t* curr_buf; // actually GuMemChunk*
	uint16_t left_edge;
	uint16_t right_edge;
	uint8_t curr_size;
	uint8_t next_size;
	bool in_stack : 1;
	uint8_t init_buf[];
};


static void*
gu_mem_alloc(size_t size) 
{
	void* p = malloc(size);
	gu_assert(p != NULL);
#ifdef GU_MEM_DEBUG
	fprintf(stderr, "malloc %u: %p\n", (unsigned) size, p);
#endif
	return p;
}
#define gu_mem_flex_new(t,m,n)			\
	((t*) gu_mem_alloc(GU_FLEX_SIZE(t,m,n)))

static void
gu_mem_free(void* p)
{
#ifdef GU_MEM_DEBUG
	fprintf(stderr, "free: %p\n", p);
#endif
	free(p);
}

GuPool* 
gu_pool_init(void* p, size_t size, bool in_stack)
{
	gu_assert(size >= offsetof(GuPool, init_buf));
	GuPool* pool = p;
	pool->curr_buf = (uint8_t*) pool;
	pool->left_edge = offsetof(GuPool, init_buf);
	pool->right_edge = size;
	pool->in_stack = in_stack;
	if (in_stack) {
		pool->curr_size = 1;
		pool->next_size = 0;
	} else {
		pool->curr_size = 0;
		pool->next_size = 1;
	}
	return pool;
}


GuPool*
gu_pool_new(void) 
{
	size_t size = GU_MEM_UNIT_SIZE - GU_MALLOC_OVERHEAD;
	void* p = gu_mem_alloc(size);
	return gu_pool_init(p, size, false);
}

static size_t
gu_pool_expansion_size(GuPool* pool, size_t alignment)
{
	size_t next_size = gu_max(pool->next_size, 1);
	size_t chunk_size = (next_size * GU_MEM_UNIT_SIZE
			     - GU_MALLOC_OVERHEAD);
	size_t offset = gu_align_forward(offsetof(GuMemChunk, data), alignment);
	return chunk_size - offset;
}

static GuMemNode
gu_pool_root_node(GuPool* pool)
{
	void* p = pool->curr_buf;
	GuMemNodeTag tag = p == pool ? GU_MEM_NODE_POOL : GU_MEM_NODE_CHUNK;
	return gu_mem_node(tag, p);
}

static void
gu_pool_expand(GuPool* pool)
{
	size_t data_size = gu_pool_expansion_size(pool, 1);
	GuMemChunk* chunk = gu_mem_flex_new(GuMemChunk, data, data_size);
	chunk->next = gu_pool_root_node(pool);
	size_t curr_size = pool->curr_size;
	pool->curr_size = pool->next_size;
	pool->next_size += curr_size;
	pool->curr_buf = (uint8_t*) chunk;
	pool->left_edge = offsetof(GuMemChunk, data);
	pool->right_edge = pool->left_edge + data_size;
}

static GuMemChunk*
gu_pool_curr_chunk(GuPool* pool)
{
	if (pool->curr_buf == (uint8_t*) pool) {
		gu_pool_expand(pool);
		gu_assert(pool->curr_buf != (uint8_t*) pool);
	}
	return (GuMemChunk*) pool->curr_buf;
}		


void*
gu_pool_malloc_large(GuPool* pool, size_t size, size_t alignment) {
	gu_assert(alignment > 0);
	size_t extra = (gu_align_forward(offsetof(GuMemChunk, data), alignment)
			- offsetof(GuMemChunk, data));
	GuMemChunk* chunk = gu_mem_flex_new(GuMemChunk, data, extra + size);
	GuMemChunk* curr = gu_pool_curr_chunk(pool);
	chunk->next = curr->next;
	curr->next = gu_mem_node(GU_MEM_NODE_DEDICATED, chunk);
	return &chunk->data[extra];
}

void*
gu_pool_malloc_aligned(GuPool* pool, size_t size, size_t alignment) 
{
	gu_assert(alignment > 0);
	gu_assert(size < GU_MEM_CHUNK_DEDICATED_THRESHOLD);
	size_t left = gu_align_forward(pool->left_edge, alignment);
	size_t avail = pool->right_edge - left;
	if (size <= avail) {
		pool->left_edge = left + size;
		return &pool->curr_buf[left];
	} else if (size <= gu_pool_expansion_size(pool, alignment)) {
		gu_pool_expand(pool);
		return gu_pool_malloc_aligned(pool, size, alignment);
	} else {
		return gu_pool_malloc_large(pool, size, alignment);
	}
}

void*
gu_pool_malloc_unaligned(GuPool* pool, size_t size)
{
	size_t right = pool->right_edge;
	size_t avail = right - pool->left_edge;
	if (size <= avail) {
		right -= size;
		pool->right_edge = right;
		return &pool->curr_buf[right];
	} else if (size <= gu_pool_expansion_size(pool, 1)) {
		gu_pool_expand(pool);
		return gu_pool_malloc_unaligned(pool, size);
	} else {
		return gu_pool_malloc_large(pool, size, 1);
	}
}

void*
gu_malloc_aligned(GuPool* pool, size_t size, size_t alignment)
{
	if (size >= GU_MEM_CHUNK_DEDICATED_THRESHOLD) {
		return gu_pool_malloc_large(pool, size, alignment);
	} else if (alignment > 1) {
		return gu_pool_malloc_aligned(pool, size, alignment);
	} else {
		return gu_pool_malloc_unaligned(pool, size);
	}
}


void 
gu_pool_finally(GuPool* pool, GuFn0* finalize)
{
	GuMemFinalizer* fin = gu_new(pool, GuMemFinalizer);
	fin->finalizer = finalize;
	GuMemChunk* curr = gu_pool_curr_chunk(pool);
	fin->next = curr->next;
	curr->next = gu_mem_node(GU_MEM_NODE_FINALIZER, fin);
}

void
gu_pool_free(GuPool* pool)
{
	GuMemChunk* chunk = NULL;
	GuMemNode node = gu_pool_root_node(pool);
	GuMemNodeTag tag;
	do {
		tag = gu_mem_node_tag(node);
		void* addr = gu_mem_node_addr(node);
		switch (tag) {
		case GU_MEM_NODE_FINALIZER: {
			// Finalizers are allocated from within chunks,
			// so don't free, just run the finalizer.
			GuMemFinalizer* fin = addr;
			node = fin->next;
			(*fin->finalizer)(fin->finalizer);
			break;
		}
		case GU_MEM_NODE_DEDICATED: {
			// Just an individually allocated chunk
			GuMemChunk* dedicated = addr;
			node = dedicated->next;
			gu_mem_free(dedicated);
			break;
		}
		case GU_MEM_NODE_CHUNK: {
			// The finalizers between this chunk and the next are
			// allocated from this chunk. Hence, keep this in
			// memory and free the previous chunk.
			if (chunk != NULL) {
				gu_mem_free(chunk);
			}
			chunk = addr;
			node = chunk->next;
			break;
		}
		case GU_MEM_NODE_POOL: {
			if (chunk != NULL) {
				gu_mem_free(chunk);
			}
		}
		}
	} while (tag != GU_MEM_NODE_POOL);
	if (!pool->in_stack) {
		gu_mem_free(pool);
	}
}


extern inline void* gu_malloc(GuPool* pool, size_t size);

extern inline void* 
gu_malloc_init_aligned(GuPool* pool, size_t size, size_t alignment, 
		       const void* init);

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
	size_t min_blocks = ((min_size + GU_MALLOC_OVERHEAD - 1) / GU_MEM_UNIT_SIZE) + 1;
	size_t blocks = gu_mem_padovan(min_blocks);
	size_t size = blocks * GU_MEM_UNIT_SIZE - GU_MALLOC_OVERHEAD;
	void* buf = realloc(old_buf, size);
	if (buf == NULL) {
		size = 0;
	}
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
	free(buf);
}
