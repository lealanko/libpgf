#ifndef GU_STR_H_
#define GU_STR_H_

#include <gu/mem.h>
#include <stdarg.h>

typedef char* GuStr;
typedef const char* GuCStr;

char* gu_str_new(int size, GuPool* pool);

char* gu_strdup(const char* str, GuPool* pool);

extern GuHashFn gu_str_hash;
extern GuEqFn gu_str_eq;

#include <gu/type.h>

extern GU_DECLARE_TYPE(GuStr, repr);


typedef wchar_t* GuWcs;
typedef const wchar_t* GuCWcs;

wchar_t* gu_wcs_new(int size, GuPool* pool);
wchar_t* gu_wcsdup(const wchar_t* wstr, GuPool* pool);

wchar_t* gu_str_wcs(const char* str, GuPool* pool);

wchar_t* gu_vasprintf(const char* fmt, va_list args, GuPool* pool);
wchar_t* gu_asprintf(GuPool* pool, const char* fmt, ...);

wchar_t* gu_vaswprintf(const wchar_t* fmt, va_list args, GuPool* pool);
wchar_t* gu_aswprintf(GuPool* pool, const wchar_t* fmt, ...);

#endif // GU_STR_H_
