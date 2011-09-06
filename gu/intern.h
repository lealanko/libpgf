#ifndef GU_INTERN_H_
#define GU_INTERN_H_

#include <gu/map.h>
#include <gu/str.h>

typedef struct GuIntern GuIntern;

GuIntern* gu_intern_new(GuPool* pool, GuPool* str_pool);
const char* gu_intern_str(GuIntern* intern, const char* cstr);

#endif /* GU_INTERN_H_ */
