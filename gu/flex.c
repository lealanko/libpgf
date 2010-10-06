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

gpointer gu_list_alloc(GuAllocator* ator, gsize base_size, gsize elem_size, 
		       gint n_elems, gsize alignment, goffset len_offset)
{
	g_assert(n_elems >= 0);
	gpointer p = gu_malloc_aligned(ator, base_size + elem_size * n_elems,
				       alignment);
	G_STRUCT_MEMBER(gint, p, len_offset) = n_elems;
	return p;
}



guint gu_string_hash(gconstpointer p)
{
	const GuString* s = p;
	// Modified from g_string_hash in glib
	gint n = s->len;
	const gchar* cp = s->elems;
	guint h = 0;

	while (n--) {
		h = (h << 5) - h + *cp;
		cp++;
	}

	return h;
}

gboolean gu_string_equal(gconstpointer p1, gconstpointer p2)
{
	const GuString* s1 = p1;
	const GuString* s2 = p2;
	if (s1->len != s2->len) {
		return FALSE;
	}
	gint cmp = memcmp(s1->elems, s2->elems, s1->len);
	return (cmp == 0);
}
