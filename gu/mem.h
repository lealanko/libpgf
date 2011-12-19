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

/** @file
 *
 * Memory allocation tools.
 */

#ifndef GU_MEM_H_
#define GU_MEM_H_

#include <gu/defs.h>
#include <gu/fun.h>

typedef struct GuFinalizer GuFinalizer;

struct GuFinalizer {
	void (*fn)(GuFinalizer* self);
};

/// @name Memory pools
/// @{

typedef struct GuPool GuPool;
/**< A memory pool. */


GU_ONLY GuPool*
gu_make_pool(void);

/**< Create a new memory pool.
 *
 */

GuPool*
gu_local_pool_(uint8_t* init_buf, size_t sz);

#define GU_LOCAL_POOL_INIT_SIZE (16 * sizeof(GuWord))

#ifdef NDEBUG
#define gu_local_pool()				\
	gu_local_pool_(gu_alloca(GU_LOCAL_POOL_INIT_SIZE),	\
		       GU_LOCAL_POOL_INIT_SIZE)
#else
#define gu_local_pool()				\
	gu_make_pool()
#endif

void gu_pool_finally(GuPool* pool, GuFinalizer* finalize);
/**< Register a function to be run when the pool is freed.
 *
 * @relates GuPool */


void
gu_pool_free(GU_ONLY GuPool* pool);
/**< Free a memory pool.
 *
 * @relates GuPool */


void
gu_pool_attach(GuPool* child, GuPool* parent);
/// @}



void* 
gu_malloc_aligned(GuPool* pool, size_t size, size_t alignment); 

/**< Allocate memory with a specified alignment.
 *
 */

void*
gu_malloc_prefixed(GuPool* pool, size_t pre_align, size_t pre_size,
		   size_t align, size_t size);


inline void*
gu_malloc(GuPool* pool, size_t size) {
	return gu_malloc_aligned(pool, size, 0);
}
/**< Allocate memory.
 *
 */


#include <string.h>

inline void* 
gu_malloc_init_aligned(GuPool* pool, size_t size, size_t alignment, 
		       const void* init)
{
	void* p = gu_malloc_aligned(pool, size, alignment);
	memcpy(p, init, size);
	return p;
}

static inline void*
gu_malloc_init(GuPool* pool, size_t size, const void* init)
{
	return gu_malloc_init_aligned(pool, size, 0, init);
}

#define gu_new_n(type, n, pool)						\
	((type*)gu_malloc_aligned((pool), sizeof(type) * (n), gu_alignof(type)))

#define gu_new(type, pool) \
	gu_new_n(type, 1, pool)

#define gu_new_prefixed(pool, pre_type, type)				\
	((type*)(gu_malloc_prefixed((pool),				\
				    gu_alignof(pre_type), sizeof(pre_type), \
				    gu_alignof(type), sizeof(type))))


	

/**< Allocate memory to store an object of a given type.
 *
 * @hideinitializer
 *
 * @param pool  The memory pool to allocate from
 * @param type  The C type of the object to allocate
 * @relates GuPool */


#ifdef GU_HAVE_STATEMENT_EXPRESSIONS
#define gu_new_i(pool, type, ...)					\
	({								\
		type *gu_new_p_ = gu_new(type, pool);			\
		memcpy((void*) gu_new_p_, &(type){ __VA_ARGS__ },	\
		       sizeof(type));					\
		gu_new_p_;						\
	})
#else // GU_HAVE_STATEMENT_EXPRESSIONS
#define gu_new_i(pool, type, ...)					\
	((type*)gu_malloc_init_aligned((pool), sizeof(type),		\
				       gu_alignof(type),		\
				       &(type){ __VA_ARGS__ }))
#endif // GU_HAVE_STATEMENT_EXPRESSIONS

#define gu_new_s gu_new_i


// Alas, there's no portable way to get the alignment of flex structs.
#define gu_flex_new(pool_, type_, flex_member_, n_elems_)		\
	((type_ *)gu_malloc_aligned(					\
		(pool_),						\
		GU_FLEX_SIZE(type_, flex_member_, n_elems_),		\
		gu_flex_alignof(type_)))



/// @name Miscellaneous
/// @{

size_t gu_mem_alignment(size_t size);

/// @}

GuPool* 
gu_pool_init(void* p, size_t len, bool in_stack);

GU_ONLY void*
gu_mem_buf_alloc(size_t min_size, size_t* real_size_out);

GU_ONLY void*
gu_mem_buf_realloc(
	GU_NULL GU_ONLY GU_RETURNED
	void* buf,
	size_t min_size,
	size_t* real_size_out);

void
gu_mem_buf_free(GU_ONLY void* buf);





#endif // GU_MEM_H_
