#include <gu/ucs.h>

#define GU_UCS_MAX INT32_C(0x10FFFF)

// TODO: check for wchar_t encoding in autoconf, or at least a
// separate config header

#if defined(__STDC_ISO_10646__)
#define WCHAR_IS_UTF32
#elif defined(_WIN32) || defined(_WIN64)
#define WCHAR_IS_UTF16
#endif


GU_DEFINE_TYPE(GuEncodingError, abstract, _);

static bool
gu_ucs_valid(GuUcs ucs)
{
	return ucs >= 0 && ucs <= GU_UCS_MAX;
}

GuUcs
gu_in_utf8(GuIn* in, GuError* err)
{
	gu_return_on_error(err, 0);
	uint8_t c = gu_in_u8(in, err);
	gu_return_on_error(err, 0);

	int len = (c < 0x80 ? 0 : 
		   c < 0xc2 ? -1 :
		   c < 0xe0 ? 1 :
		   c < 0xf0 ? 2 :
		   c < 0xf5 ? 3 :
		   -1);
	if (len < 0) {
		goto fail;
	}

	static const uint8_t mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };
	GuUcs ucs = c & mask[len];

	for (int i = 0; i < len; i++) {
		c = gu_in_u8(in, err);
		gu_return_on_error(err, 0);
		if ((c & 0xc0) != 0x80) {
			gu_raise_null(err, GuEncodingError);
			return 0;
		}
		ucs = ucs << 6 | (c & 0x3f);
	}
	if (!gu_ucs_valid(ucs)) {
		goto fail;
	}
	return ucs;

fail:
	gu_raise_null(err, GuEncodingError);
	return 0;
}

void
gu_out_utf8(GuOut* out, GuUcs ucs, GuError* err)
{
	gu_require(gu_ucs_valid(ucs));
	if (ucs < 0x80) {
		gu_out_u8(out, ucs, err);
		return;
	}
	
	int len = (ucs < 0x800 ? 1 :
		   ucs < 0x10000L ? 2 :
		   3);
	uint8_t buf[4];
	for (int i = len; i > 0; i--) {
		buf[i] = 0x80 | (ucs & 0x3f);
		ucs >>= 6;
	}
	buf[0] = (uint8_t) (0xf80 >> len) | ucs;
	
	int idx = 4;
	while (ucs) {
		buf[--idx] = 
		ucs >>= 6;
	}
	gu_out_bytes(out, buf, len + 1, err);
}

#ifdef WCHAR_IS_UTF32

void
gu_write_ucs(GuWriter* wtr, GuUcs ucs, GuError* err)
{
	gu_require(gu_ucs_valid(ucs));
	gu_putc(wtr, (wchar_t) ucs, err);
}

GuUcs
gu_read_ucs(GuReader* rdr, GuError* err)
{
	wint_t wc = gu_getc(rdr, err);
	if (wc == WEOF) {
		return GU_UCS_EOF;
	}
	return (GuUcs) (wchar_t) wc;
}

#elif defined WCHAR_IS_UTF16

void
gu_write_ucs(GuWriter* wtr, GuUcs ucs, GuError* err)
{
	gu_require(gu_ucs_valid(ucs));
	if (ucs < 0x10000L) {
		gu_putc(wtr, (wchar_t) ucs, err);
	} else {
		uint32_t u = ucs - 0x10000L;
		wchar_t wcs[2] = { 0xd800 | (u >> 10), 
				   0xdc00 | (u & 0x03ff) };
		gu_write(wtr, wcs, 2, err);
	}
}

GuUcs
gu_read_ucs(GuReader* rdr, GuError* err)
{
	wint_t wc = gu_getc(rdr, err);
	if (wc == WEOF) {
		return GU_UCS_EOF;
	} else if (wc < 0xd800 || wc >= 0xe000) {
		return (GuUcs) wc;
	} else if (wc >= 0xdc00) {
		gu_raise_null(err, GuEncodingError);
		return GU_UCS_EOF;
	}
	wint_t wc2 = gu_getc(rdr, err);
	if (wc2 == WEOF) {
		return GU_UCS_EOF;
	}
	if (wc2 < 0xdc00 || wc >= 0xe000) {
		gu_raise_null(err, GuEncodingError);
	}
	int32_t i1 = wc;
	int32_t i2 = wc2;
	return ((i1 & 0x03ff) << 10 | (i2 & 0x3ff));
}

#endif


#if 0
GuUtf8
gu_wcs_utf8(wchar_t* wcs, GuPool* pool)
{
	GuPool* tmp_pool = gu_pool_new();
	GuByteSeq byteq = gu_byte_seq_new(tmp_pool);
	GuOut* out = gu_byte_seq_out(byteq, tmp_pool);
	GuError* err = gu_error_new(tmp_pool);
	
}

wchar_t*
gu_utf8_wcs(GuCUtf8 utf8, GuPool* pool)
{
	GuPool* tmp_pool = gu_pool_new();
	GuByteSeq byteq = gu_byte_seq_new(tmp_pool);
	GuOut* out = gu_byte_seq_out(byteq, tmp_pool);
	GuError* err = gu_error_new(tmp_pool);
	
}

#endif
