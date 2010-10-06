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

#ifndef GU_FLEX_H_
#define GU_FLEX_H_

#include "mem.h"

#ifdef GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#define gu_flex_alignof gu_alignof
#else
#define gu_flex_alignof(t) 0
#endif

#define GU_FLEX_SIZE(type, flex_member, n_elems) \
	(sizeof(type) + ((n_elems) * sizeof(((type *)NULL)->flex_member[0])))


// Alas, there's no portable way to get the alignment of flex structs.
#define gu_flex_new(ator, type, len_member, flex_member, n_elems)	\
	(((type)*)gu_malloc((ator),					\
			    GU_FLEX_SIZE(type, flex_member, n_elems),	\
			    gu_flex_alignof(t)))


#define GuList(t)	   \
	struct {	   \
		gint len;  \
		t elems[]; \
	}

gpointer gu_list_alloc(GuAllocator* ator, gsize base_size, gsize elem_size, 
		       gint n_elems, gsize alignment, goffset len_offset);

#define gu_list_new(t, ator, n)						\
	((t*) gu_list_alloc(ator,					\
			    sizeof(t),					\
			    sizeof(((t*)NULL)->elems[0]),		\
			    (n),					\
			    gu_flex_alignof(t),				\
			    offsetof(t, len)))


#define gu_list_length(lst) \
	((lst)->len)

#define gu_list_elems(lst) \
	((lst)->elems)

typedef GuList(gpointer) GuPointers; 
typedef GuList(guint8) GuBytes;
typedef GuList(gchar) GuString;
typedef GuList(gint) GuInts;		      
typedef GuList(GuString*) GuStrings;
		            		      
guint gu_string_hash(gconstpointer s);
gboolean gu_string_equal(gconstpointer s1, gconstpointer s2);

#endif // GU_FLEX_H_
