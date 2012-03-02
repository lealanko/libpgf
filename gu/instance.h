#ifndef GU_INSTANCE_H_
#define GU_INSTANCE_H_

#include <gu/type.h>

typedef GuTypeMap GuInstanceMap;
typedef const struct GuInstance GuInstance;

struct GuInstance {
	const void* (*make)(GuInstance* self, GuInstanceMap* imap,
		      GuType* type, GuPool* pool);
};

#define GU_INSTANCE(MAKE_FN)			\
	(&(GuInstance){ MAKE_FN })

typedef const struct GuConstInstance GuConstInstance;
struct GuConstInstance {
	GuInstance instance;
	const void* const_value;
};

const void*
gu_const_instance_make(GuInstance* self, GuInstanceMap* imap,
		       GuType* type, GuPool* pool);

#define GU_CONST_INSTANCE(VALUE)					\
	(&(GuConstInstance) {						\
		.instance =  { gu_const_instance_make },	\
		.const_value = (VALUE)			\
	 })

const void*
gu_instantiate(GuInstanceMap* imap, GuType* type, GuPool* pool);


#endif /* GU_INSTANCE_H_ */
