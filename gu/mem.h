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

/// @name Memory pools
/// @{

typedef struct GuPool GuPool;
/**< A memory pool. */

GuPool* gu_pool_new(void);
/**< Create a new memory pool.
 *
 */


void gu_pool_finally(GuPool* pool, GDestroyNotify func, gpointer p);
/**< Register a function to be run when the pool is freed.
 *
 * @relates GuPool */


void gu_pool_free(GuPool* pool);
/**< Free a memory pool.
 *
 * @relates GuPool */

/// @}


gpointer 
gu_malloc_aligned(GuPool* pool, gsize size, gsize alignment);
/**< Allocate memory with a specified alignment.
 *
 */

inline gpointer
gu_malloc(GuPool* pool, gsize size) {
	return gu_malloc_aligned(pool, size, 0);
}
/**< Allocate memory.
 *
 */

#define gu_new(pool, type) \
	((type*)gu_malloc_aligned((pool), sizeof(type), gu_alignof(type)))
/**< Allocate memory to store an object of a given type.
 *
 * @hideinitializer
 *
 * @param pool  The memory pool to allocate from
 * @param type  The C type of the object to allocate
 * @relates GuPool */


/// @name Miscellaneous
/// @{

gsize gu_mem_alignment(gsize size);

/// @}


G_END_DECLS

#endif // GU_MEM_H_
