#ifndef GU_UTF8_H_
#define GU_UTF8_H_

#include <gu/in.h>
#include <gu/out.h>
#include <gu/ucs.h>
#include <gu/read.h>

GuUCS
gu_in_utf8(GuIn* in, GuError* err);

GuReader*
gu_utf8_reader(GuIn* in, GuPool* pool);

void
gu_out_utf8_long_(GuUCS ucs, GuOut* out, GuError* err);

inline void
gu_out_utf8(GuUCS ucs, GuOut* out, GuError* err)
{
	gu_require(gu_ucs_valid(ucs));
	if (GU_LIKELY(ucs < 0x80)) {
		gu_out_u8(out, ucs, err);
	} else {
		gu_out_utf8_long_(ucs, out, err);
	}
}

size_t
gu_utf32_out_utf8(const GuUCS* src, size_t len, GuOut* out, GuError* err);

GuUCS
gu_utf8_decode(const uint8_t** utf8);

inline void 
gu_str_out_utf8(const char* str, GuOut* out, GuError* err)
{
#ifdef GU_CHAR_ASCII
	gu_out_bytes(out, (const uint8_t*) str, strlen(str), err);
#else
	extern void 
		gu_str_out_utf8_(const char* str, GuOut* out, GuError* err);
	gu_str_out_utf8_(str, out, err);
#endif
}

#endif // GU_UTF8_H_
