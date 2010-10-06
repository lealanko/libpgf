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

#include "variant.h"

enum {
	ALIGNMENT = sizeof(guintptr)
};

gpointer 
gu_variant_alloc(GuAllocator* ator, guint8 tag, gsize size, 
		 gsize align, GuVariant* variant_out)
{
	if (align == 0) {
		align = gu_mem_alignment(size);
	}
	align = MAX(ALIGNMENT, align);
	if (((gsize)tag) > ALIGNMENT - 2) {
		guint8* alloc = gu_malloc_aligned(ator, align + size, align);
		alloc[align - 1] = tag;
		gpointer p = &alloc[align];
		variant_out->p = (guintptr)p;
		return p;
	}
	gpointer p = gu_malloc_aligned(ator, size, align);
	variant_out->p = ((guintptr)p) | (tag + 1);
	return p;
}

guint
gu_variant_tag(GuVariant variant)
{
	guint u = variant.p % ALIGNMENT;
	if (u == 0) {
		guint8* mem = (guint8*)variant.p;
		return mem[-1];
	}
	return u - 1;
}

gpointer
gu_variant_data(GuVariant variant)
{
	return (gpointer)((variant.p / ALIGNMENT) * ALIGNMENT);
}

gint 
gu_variant_intval(GuVariant variant)
{
	guint u = variant.p % ALIGNMENT;
	if (u == 0) {
		gint* mem = (gint*)variant.p;
		return *mem;
	}
	return (variant.p / ALIGNMENT);
}


struct foo {
	gint l;
	struct {
		gint u;
		gchar arr[];
	} fin;
};
