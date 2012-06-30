// Copyright 2012 University of Helsinki. Released under LGPL3.

#include <gu/generic.h>


const void*
gu_const_specialize_(GuInstance* self, GuGeneric* gen,
		     GuType* type, GuPool* pool)
{
	GuConstInstance* ci = (GuConstInstance*) self;
	return ci->const_value;
}


GuGeneric*
gu_new_generic(GuTypeTable* instances, GuPool* pool)
{
	GuTypeMap* imap = gu_new_type_map(instances, pool);
	return (GuGeneric*) imap;
}

const void*
gu_specialize(GuGeneric* gen, GuType* type, GuPool* pool)
{
	GuTypeMap* imap = (GuTypeMap*) gen;
	GuInstance* inst = gu_type_map_get(imap, type);
	return inst->specialize(inst, gen, type, pool);
}
