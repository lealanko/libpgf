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

#ifndef GU_VARIANT_H_
#define GU_VARIANT_H_

#include "mem.h"

typedef struct GuVariant GuVariant;

gpointer gu_variant_alloc(GuAllocator* ator, guint8 tag, 
			  gsize size, gsize align, 
			  GuVariant* variant_out);

#define gu_variant_new(ator, tag, type, variant_out) \
	((type*)gu_variant_alloc(ator, tag, sizeof(type), variant_out))

#define gu_variant_flex_new(ator, tag, type, flex_mem, n_elems, variant_out)	\
	((type*)gu_variant_alloc(ator, tag,				\
				 GU_FLEX_SIZE(type, flex_mem, n_elems), \
				 variant_out)

guint gu_variant_tag(GuVariant variant);

gpointer gu_variant_data(GuVariant variant);


// <private>

struct GuVariant {
	guintptr p;
};

#endif // GU_VARIANT_H_
