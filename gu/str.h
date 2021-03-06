// Copyright 2011 University of Helsinki. Released under LGPL3.

#ifndef GU_STR_H_
#define GU_STR_H_

#include <gu/mem.h>
#include <gu/hash.h>

extern const char gu_empty_str[];
extern const char* const gu_null_str;

typedef const char* GuStr;

char* gu_new_str(size_t size, GuPool* pool);

char* gu_strdup(const char* str, GuPool* pool);

bool
gu_str_eq(GuStr s1, GuStr s2);

extern GuHasher gu_str_hasher[1];

#include <gu/type.h>

extern GU_DECLARE_TYPE(GuStr, repr);

char* gu_vasprintf(const char* fmt, va_list args, GuPool* pool);

char* gu_asprintf(GuPool* pool, const char* fmt, ...);

inline GuSlice gu_str_slice(char* str)
{
	return (GuSlice) { (uint8_t*) str, strlen(str) };
}

inline GuCSlice gu_str_cslice(const char* cstr)
{
	return (GuCSlice) { (const uint8_t*) cstr, strlen(cstr) };
}

#endif // GU_STR_H_
