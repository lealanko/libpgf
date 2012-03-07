#ifndef GU_ENUM_H_
#define GU_ENUM_H_

#include <gu/mem.h>

typedef struct GuEnum GuEnum;

struct GuEnum {
	bool (*next)(GuEnum* self, void* to, GuPool* pool);
};

bool
gu_enum_next(GuEnum* en, void* to, GuPool* pool);


#endif /* GU_ENUM_H_ */
