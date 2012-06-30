// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <gu/enum.h>

bool
gu_enum_next(GuEnum* en, void* to, GuPool* pool)
{
	return en->next(en, to, pool);
}
