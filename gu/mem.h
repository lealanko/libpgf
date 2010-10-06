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

#include <glib.h>
#include "macros.h"

G_BEGIN_DECLS


/** @name Allocators
 *
 * @{ */

typedef struct GuAllocator GuAllocator;
/**< An allocator. */


gpointer gu_malloc_aligned(GuAllocator* ator, gsize size, gsize alignment);
/**< Allocate memory with a specified alignment.
 *
 */


inline gpointer
gu_malloc(GuAllocator* ator, gsize size) {
	return gu_malloc_aligned(ator, size, 0);
}
/**< Allocate memory.
 *
 */

#define gu_new(ator, type) \
	((type*)gu_malloc_aligned((ator), sizeof(type), gu_alignof(type)))
/**< Allocate memory to store an object of a given type.
 *
 * @hideinitializer
 *
 * @param ator  The GuAllocator to allocate from
 * @param type  The C type of the object to allocate
 * @relates GuAllocator */

GuAllocator* gu_slice_allocator(void);
/**< Allocator that uses #g_slice_alloc
 */



GuAllocator* gu_malloc_allocator(void);
/**< Allocator that uses #g_malloc.
 *
 */

/// @}

/// @name Memory pools
/// @{

typedef struct GuMemPool GuMemPool;
/**< A memory pool. */

GuMemPool* gu_mem_pool_new(void);
/**< Create a new memory pool.
 *
 */


GuAllocator* gu_mem_pool_allocator(GuMemPool* pool);
/**< Allocator for this memory pool.
 *
 * @relates GuMemPool */


void gu_mem_pool_register_finalizer(GuMemPool* pool, 
				    GDestroyNotify func, gpointer p);
/**< Register a function to be run when the pool is freed.
 *
 * @relates GuMemPool */


void gu_mem_pool_free(GuMemPool* pool);
/**< Free a memory pool.
 *
 * @relates GuMemPool */

/// @}

/// @name Miscellaneous
/// @{

gsize gu_mem_alignment(gsize size);

/// @}


G_END_DECLS

#endif // GU_MEM_H_
