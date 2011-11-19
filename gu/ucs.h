#ifndef GU_UCS_H_
#define GU_UCS_H_

#include <gu/str.h>
#include <gu/in.h>
#include <gu/out.h>
#include <gu/write.h>
#include <gu/read.h>

#define GU_UCS_EOF ((GuUcs) INT32_C(-1))

typedef int32_t GuUcs;
typedef uint8_t* GuUtf8;
typedef const uint8_t* GuCUtf8;

GuUcs
gu_in_utf8(GuIn* in, GuError* err);

//GuUcs
//gu_read_ucs(GuReader* rdr, GuError* err);

void
gu_out_utf8(GuOut* out, GuUcs ucs, GuError* err);

void
gu_write_ucs(GuWriter* wtr, GuUcs ucs, GuError* err);

GuUcs
gu_in_utf8(GuIn* in, GuError* err);

GuUcs
gu_read_ucs(GuReader* rdr, GuError* err);

GuUtf8
gu_wcs_utf8(wchar_t* wcs, GuPool* pool);

wchar_t*
gu_utf8_wcs(GuCUtf8 utf8, GuPool* pool);

extern GU_DECLARE_TYPE(GuEncodingError, abstract);

#endif // GU_ISO10646_H_
