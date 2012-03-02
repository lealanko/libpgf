#include <gu/instance.h>


const void*
gu_const_instance_make(GuInstance* self, GuInstanceMap* imap,
		       GuType* type, GuPool* pool)
{
	GuConstInstance* ci = (GuConstInstance*) self;
	return ci->const_value;
}


const void*
gu_instantiate(GuInstanceMap* imap, GuType* type, GuPool* pool)
{
	GuInstance* inst = gu_type_map_get(imap, type);
	if (!inst) {
		return NULL;
	}
	return inst->make(inst, imap, type, pool);
}
