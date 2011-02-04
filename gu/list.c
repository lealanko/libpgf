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

#include "list.h"

#include <string.h>

static const int gu_list_empty = 0;

void* gu_list_alloc(GuPool* pool, size_t base_size, size_t elem_size, 
		       int n_elems, size_t alignment, ptrdiff_t len_offset)
{
	g_assert(n_elems >= 0);
	if (n_elems == 0 && len_offset == 0) {
		return (void*) &gu_list_empty;
	}
	void* p = gu_malloc_aligned(pool, base_size + elem_size * n_elems,
				       alignment);
	G_STRUCT_MEMBER(int, p, len_offset) = n_elems;
	return p;
}


GU_DEFINE_KIND(GuList, abstract);

GU_DEFINE_TYPE(GuStrings, GuList, gu_type(GuStringP));
GU_DEFINE_TYPE(GuStringsP, pointer, gu_type(GuStrings));

