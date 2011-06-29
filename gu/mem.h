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

/// @name Memory pools
/// @{

typedef struct GuPool GuPool;
/**< A memory pool. */

GuPool* gu_pool_new(void);
/**< Create a new memory pool.
 *
 */


void gu_pool_finally(GuPool* pool, GuFn0* finalize);
/**< Register a function to be run when the pool is freed.
 *
 * @relates GuPool */


void gu_pool_free(GuPool* pool);
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

#define gu_new(pool, type) \
	((type*)gu_malloc_aligned((pool), sizeof(type), gu_alignof(type)))

/**< Allocate memory to store an object of a given type.
 *
 * @hideinitializer
 *
 * @param pool  The memory pool to allocate from
 * @param type  The C type of the object to allocate
 * @relates GuPool */


#ifdef GU_HAVE_STATEMENT_EXPRESSIONS
#define gu_new_s(pool, type, ...)			\
	({						\
		type *p_ = gu_new(pool, type);		\
		memcpy((void*) p_, &(type){ __VA_ARGS__ }, sizeof(type)); \
		p_;					\
	})
#else // GU_HAVE_STATEMENT_EXPRESSIONS
#define gu_new_s(pool, type, ...)					\
	((type*)gu_malloc_init_aligned((pool), sizeof(type),		\
				       gu_alignof(type),		\
				       &(type){ __VA_ARGS__ }))
#endif // GU_HAVE_STATEMENT_EXPRESSIONS




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

void*
gu_mem_buf_alloc(size_t min_size, size_t* real_size_out);

void*
gu_mem_buf_realloc(void* buf, size_t min_size, size_t* real_size_out);

void
gu_mem_buf_free(void* buf);





#endif // GU_MEM_H_
