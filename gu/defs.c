// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

#include <gu/defs.h>
#include <string.h>

void* const gu_null = NULL;
GuStruct* const gu_null_struct = NULL;

bool
gu_cslice_eq(GuCSlice s1, GuCSlice s2)
{
	return (s1.sz == s2.sz
		&& (s1.p == s2.p
		    || (s1.p && s2.p
			&& memcmp(s1.p, s2.p, s1.sz) == 0)));
}

extern inline GuSlice
gu_slice(uint8_t* p, size_t sz);

extern inline GuSlice
gu_null_slice(void);

extern inline GuCSlice
gu_cslice(const uint8_t* p, size_t sz);

extern inline GuCSlice
gu_null_cslice(void);

extern inline GuCSlice
gu_slice_cslice(GuSlice slice);

