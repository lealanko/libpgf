// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef GU_UCS_H_
#define GU_UCS_H_

#include <gu/defs.h>
#include <gu/exn.h>
#include <gu/assert.h>


#if defined(__STDC_ISO_10646__) && WCHAR_MAX >= 0x10FFFF
#include <wchar.h>
/// If defined, #GuUCS is a `typedef` for `wchar_t`.
#define GU_UCS_WCHAR
typedef wchar_t GuUCS;
#else
typedef int32_t GuUCS;
#endif

/**< A Unicode code point value.
 */

#define GU_UCS_MAX ((GuUCS)(0x10FFFF))

/// Check whether a C character is portable.
bool
gu_char_is_valid(char c);
/**< @return `true` iff `c` belongs to the basic execution character set of
 * the C environment.
 */

inline bool
gu_ucs_valid(GuUCS ucs)
{
	return ucs >= 0 && ucs <= GU_UCS_MAX;
}

/** Convert a C character into a code point.
 *
 * @param c A character from the basic execution character set.
 *
 * @return The Unicode code point corresponding to the character `c`.
 */

inline GuUCS
gu_char_ucs(char c)
{
	gu_require(gu_char_is_valid(c));
#ifdef GU_CHAR_ASCII
	GuUCS u = (GuUCS) c;
#else
	extern const uint8_t gu_ucs_ascii_reverse_[CHAR_MAX];
	GuUCS u = gu_ucs_ascii_reverse_[(unsigned char) c];
#endif
	gu_ensure(u < 0x80);
	return u;
}

/// Convert a code point into a C character
char
gu_ucs_char(GuUCS uc);

/// Convert a C string into a code point sequence
size_t
gu_str_to_ucs(const char* cbuf, size_t len, GuUCS* ubuf, GuExn* err);

/// Convert a code point sequence into a C string
size_t
gu_ucs_to_str(const GuUCS* ubuf, size_t len, char* cbuf, GuExn* err);

extern GU_DECLARE_TYPE(GuUCSExn, abstract);

#endif // GU_ISO10646_H_
