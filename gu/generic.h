#ifndef GU_INSTANCE_H_
#define GU_INSTANCE_H_

#include <gu/type.h>

typedef struct GuGeneric GuGeneric;
typedef const struct GuInstance GuInstance;

struct GuInstance {
	const void* (*specialize)(GuInstance* self, GuGeneric* gen,
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
gu_const_specialize_(GuInstance* self, GuGeneric* gen,
		     GuType* type, GuPool* pool);

#define GU_CONST_INSTANCE(VALUE)					\
	(&(GuConstInstance) {						\
		.instance =  { gu_const_specialize_ },	\
		.const_value = (VALUE)			\
	 })

GuGeneric*
gu_new_generic(GuTypeTable* instances, GuPool* pool);

const void*
gu_specialize(GuGeneric* generic, GuType* type, GuPool* pool);


#endif /* GU_INSTANCE_H_ */
