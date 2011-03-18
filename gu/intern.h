#ifndef GU_INTERN_H_
#define GU_INTERN_H_
#include <gu/map.h>
#include <gu/string.h>

typedef struct GuIntern GuIntern;

GuIntern* gu_intern_new(GuPool* pool, GuPool* str_pool);
GuString* gu_intern_string(GuIntern* intern, GuCString* cstr);

#endif /* GU_INTERN_H_ */
