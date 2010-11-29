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
 * Utilities for structures with flexible array members.
 */

#ifndef GU_FLEX_H_
#define GU_FLEX_H_

#include <gu/mem.h>

#ifdef GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#define gu_flex_alignof gu_alignof
#else
#define gu_flex_alignof(t) 0
#endif

#define GU_FLEX_SIZE(type, flex_member, n_elems) \
	(sizeof(type) + ((n_elems) * sizeof(((type *)NULL)->flex_member[0])))
/**< @hideinitializer */


// Alas, there's no portable way to get the alignment of flex structs.
#define gu_flex_new(pool, type, len_member, flex_member, n_elems)	\
	(((type)*)gu_malloc((pool),					\
			    GU_FLEX_SIZE(type, flex_member, n_elems),	\
			    gu_flex_alignof(t)))


#define GuList(t)	   \
	struct {	   \
		int len;  \
		t elems[]; \
	}

void* gu_list_alloc(GuPool* pool, size_t base_size, size_t elem_size, 
		    int n_elems, size_t alignment, ptrdiff_t len_offset);

#define gu_list_new(t, pool, n)						\
	((t*) gu_list_alloc(pool,					\
			    sizeof(t),					\
			    sizeof(((t*)NULL)->elems[0]),		\
			    (n),					\
			    gu_flex_alignof(t),				\
			    offsetof(t, len)))


#define gu_list_length(lst) \
	((lst)->len)

#define gu_list_elems(lst) \
	((lst)->elems)

typedef GuList(void*) GuPointers; 
typedef GuList(uint8_t) GuBytes;
typedef GuList(gchar) GuString;
typedef GuList(int) GuInts;		      
typedef GuList(GuString*) GuStrings;
		            		      
unsigned gu_string_hash(const void* s);
gboolean gu_string_equal(const void* s1, const void* s2);

#define GuSList(t)				\
	const struct {				\
		int len;			\
		t* elems;			\
	}

#define GU_SLIST_0 { .len = 0, .elems = NULL }

#define GU_SLIST(t, ...)						\
	{								\
		.len = sizeof((t[]){__VA_ARGS__}) / sizeof(t),	\
		.elems = ((t[]){__VA_ARGS__})			\
	}

#endif // GU_FLEX_H_
