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

#include "flex.h"

#include <string.h>

void* gu_list_alloc(GuPool* pool, size_t base_size, size_t elem_size, 
		       int n_elems, size_t alignment, ptrdiff_t len_offset)
{
	g_assert(n_elems >= 0);
	void* p = gu_malloc_aligned(pool, base_size + elem_size * n_elems,
				       alignment);
	G_STRUCT_MEMBER(int, p, len_offset) = n_elems;
	return p;
}



unsigned gu_string_hash(const void* p)
{
	const GuString* s = p;
	// Modified from g_string_hash in glib
	int n = s->len;
	const gchar* cp = s->elems;
	unsigned h = 0;

	while (n--) {
		h = (h << 5) - h + *cp;
		cp++;
	}

	return h;
}

gboolean gu_string_equal(const void* p1, const void* p2)
{
	const GuString* s1 = p1;
	const GuString* s2 = p2;
	if (s1->len != s2->len) {
		return FALSE;
	}
	int cmp = memcmp(s1->elems, s2->elems, s1->len);
	return (cmp == 0);
}
