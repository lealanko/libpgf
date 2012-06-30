// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

#include <gu/list.h>
#include <gu/assert.h>
#include <string.h>

static const int gu_list_empty = 0;

void* gu_list_alloc(GuPool* pool, size_t base_size, size_t elem_size, 
		       int n_elems, size_t alignment)
{
	gu_assert(n_elems >= 0);
	if (n_elems == 0) {
		return (void*) &gu_list_empty;
	}
	// XXX: use gu_flex_size, use offset of elems
	void* p = gu_malloc_aligned(pool, base_size + elem_size * n_elems,
				    alignment);
	*(int*) p = n_elems;
	return p;
}


GU_DEFINE_KIND(GuList, abstract);

// GU_DEFINE_TYPE(GuStrs, GuList, gu_type(GuStr));
// GU_DEFINE_TYPE(GuStrsP, pointer, gu_type(GuStrs));

void*
gu_list_type_alloc(GuListType* ltype, int n_elems, GuPool* pool)
{
	return gu_list_alloc(pool, ltype->size, 
			     gu_type_size(ltype->elem_type),
			     n_elems, ltype->align);
}

void*
gu_list_type_index(GuListType* ltype, void* list, int i)
{
	uint8_t* p = list;
	return &p[ltype->elems_offset + i * gu_type_size(ltype->elem_type)];
}
