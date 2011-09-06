#ifndef GU_STR_H_
#define GU_STR_H_

#include <gu/mem.h>
#include <stdarg.h>

typedef char* GuStr;
typedef const char* GuCStr;

char* gu_str_alloc(int size, GuPool* pool);

char* gu_strdup(const char* str, GuPool* pool);

char* gu_vasprintf(const char* fmt, va_list args, GuPool* pool);

char* gu_asprintf(GuPool* pool, const char* fmt, ...);

extern GuHashFn gu_str_hash;
extern GuEqFn gu_str_eq;

#include <gu/type.h>

extern GU_DECLARE_TYPE(GuStr, repr);

#endif // GU_STR_H_
