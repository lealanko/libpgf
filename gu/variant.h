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
 * Lightweight tagged data.    
 */

#ifndef GU_VARIANT_H_
#define GU_VARIANT_H_

#include "mem.h"

/** @name Variants
 * @{
 */

typedef struct GuVariant GuVariant;


gpointer gu_variant_alloc(GuPool* pool, guint8 tag, 
			  gsize size, gsize align, 
			  GuVariant* variant_out);


#define gu_variant_new(pool, tag, type, variant_out)	  \
	((type*)gu_variant_alloc(pool, tag, sizeof(type), \
				 gu_alignof(type), variant_out))
/**< 
 * @hideinitializer */


#define gu_variant_flex_new(ator, tag, type, flex_mem, n_elems, variant_out)	\
	((type*)gu_variant_alloc(ator, tag,				\
				 GU_FLEX_SIZE(type, flex_mem, n_elems), \
				 gu_flex_alignof(type),			\
				 variant_out))
/**< 
 * @hideinitializer */

enum {
	GU_VARIANT_NULL = (guint)-1
};

guint gu_variant_tag(GuVariant variant);

gpointer gu_variant_data(GuVariant variant);


typedef struct GuVariantInfo GuVariantInfo;

struct GuVariantInfo {
	guint tag;
	gpointer data;
};

GuVariantInfo gu_variant_open(GuVariant variant);

/** @privatesection */
struct GuVariant {
	guintptr p;
	/**< @private */
};

/** @} */

inline gpointer
gu_variant_to_ptr(GuVariant variant)
{
	return GINT_TO_POINTER(variant.p);
}

inline GuVariant
gu_variant_from_ptr(gpointer p)
{
	GuVariant v = { GPOINTER_TO_INT(p) };
	return v;
}

extern GuVariant gu_variant_null;

inline gboolean
gu_variant_is_null(GuVariant v) {
	return (v.p == (guintptr) NULL);
}

#endif // GU_VARIANT_H_
