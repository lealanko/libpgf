#ifndef GU_UCS_H_
#define GU_UCS_H_

#include <gu/defs.h>
#include <gu/error.h>

#if defined(__STDC_ISO_10646__) && WCHAR_MAX >= 0x10FFFF
#include <wchar.h>
#define GU_UCS_WCHAR
typedef wchar_t GuUCS;
#else
typedef int32_t GuUCS;
#endif

#define GU_UCS_MAX ((GuUCS)(0x10FFFF))

bool
gu_char_is_valid(char c);

static inline bool
gu_ucs_valid(GuUCS ucs)
{
	return ucs >= 0 && ucs <= GU_UCS_MAX;
}

GuUCS
gu_char_ucs(char c);

char
gu_ucs_char(GuUCS uc, GuError* err);

size_t
gu_str_to_ucs(const char* cbuf, size_t len, GuUCS* ubuf, GuError* err);

size_t
gu_ucs_to_str(const GuUCS* ubuf, size_t len, char* cbuf, GuError* err);

extern GU_DECLARE_TYPE(GuUCSError, abstract);

#endif // GU_ISO10646_H_