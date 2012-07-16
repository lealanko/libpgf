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
