#include <gu/assert.h>
#include <gu/utf8.h>
#include "config.h"

GuUCS
gu_utf8_decode(const uint8_t** src_inout)
{
	const uint8_t* src = *src_inout;
	uint8_t c = src[0];
	if (c < 0x80) {
		*src_inout = src + 1;
		return (GuUCS) c;
	}
	size_t len = (c < 0xe0 ? 1 :
		      c < 0xf0 ? 2 :
		      3);
	uint32_t mask = 0x07071f7f;
	uint32_t u = c & (mask >> (len * 8));
	for (size_t i = 1; i <= len; i++) {
		c = src[i];
		u = u << 6 | (c & 0x3f);
	}
	*src_inout = &src[len + 1];
	return (GuUCS) u;
}

GuUCS
gu_in_utf8(GuIn* in, GuError* err)
{
	uint8_t c = gu_in_u8(in, err);
	if (!gu_ok(err)) {
		return 0;
	}
	int len = (c < 0x80 ? 0 : 
		   c < 0xc2 ? -1 :
		   c < 0xe0 ? 1 :
		   c < 0xf0 ? 2 :
		   c < 0xf5 ? 3 :
		   -1);
	if (len < 0) {
	 	goto fail;
	} else if (len == 0) {
		return c;
	}
	static const uint8_t mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };
	uint32_t u = c & mask[len];
	uint8_t buf[3];
	// If reading the extra bytes causes EOF, it is an encoding
	// error, not a legitimate end of character stream.
	GuError* tmp_err = gu_error(err, GuEOF, NULL);
	gu_in_bytes(in, buf, len, tmp_err);
	if (tmp_err->caught) {
		goto fail;
	}
	if (!gu_ok(err)) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		c = buf[i];
		if ((c & 0xc0) != 0x80) {
			goto fail;
		}
		u = u << 6 | (c & 0x3f);
	}
	GuUCS ucs = (GuUCS) u;
	if (!gu_ucs_valid(ucs)) {
		goto fail;
	}
	return ucs;

fail:
	gu_raise(err, GuUCSError);
	return 0;
}


size_t
gu_advance_utf8(GuUCS ucs, uint8_t* buf)
{
	gu_require(gu_ucs_valid(ucs));
	if (ucs < 0x80) {
		buf[0] = (uint8_t) ucs;
		return 1;
	} else if (ucs < 0x800) {
		buf[0] = 0xc0 | (ucs >> 6);
		buf[1] = 0x80 | (ucs & 0x3f);
		return 2;
	} else if (ucs < 0x10000) {
		buf[0] = 0xe0 | (ucs >> 12);
		buf[1] = 0x80 | ((ucs >> 6) & 0x3f);
		buf[2] = 0x80 | (ucs & 0x3f);
		return 3;
	} else {
		buf[0] = 0xf0 | (ucs >> 18);
		buf[1] = 0x80 | ((ucs >> 12) & 0x3f);
		buf[2] = 0x80 | ((ucs >> 6) & 0x3f);
		buf[3] = 0x80 | (ucs & 0x3f);
		return 4;
	}
}

void
gu_out_utf8_long_(GuUCS ucs, GuOut* out, GuError* err)
{
	uint8_t buf[4];
	size_t sz = gu_advance_utf8(ucs, buf);
	switch (sz) {
	case 2:
		gu_out_bytes(out, buf, 2, err);
		break;
	case 3:
		gu_out_bytes(out, buf, 3, err);
		break;
	case 4:
		gu_out_bytes(out, buf, 4, err);
		break;
	default:
		gu_impossible();
	}
}

extern inline void
gu_out_utf8(GuUCS ucs, GuOut* out, GuError* err);

static size_t 
gu_utf32_out_utf8_buffered_(const GuUCS* src, size_t len, GuOut* out, 
			    GuError* err)
{
	size_t src_i = 0;
	while (src_i < len) {
		size_t dst_sz;
		uint8_t* dst = gu_out_begin_span(out, &dst_sz);
		if (!dst) {
			gu_out_utf8(src[src_i], out, err);
			if (!gu_ok(err)) {
				return src_i;
			}
			src_i++;
			break;
		} 
		size_t dst_i = 0;
		while (true) {
			size_t safe = (dst_sz - dst_i) / 4;
			size_t end = GU_MIN(len, src_i + safe);
			if (end == src_i) {
				break;
			}
			do {
				GuUCS ucs = src[src_i++];
				dst_i += gu_advance_utf8(ucs, &dst[dst_i]);
			} while (src_i < end);
		} 
		gu_out_end_span(out, dst_i);
	}
	return src_i;
}

size_t
gu_utf32_out_utf8(const GuUCS* src, size_t len, GuOut* out, GuError* err)
{
	if (gu_out_is_buffered(out)) {
		return gu_utf32_out_utf8_buffered_(src, len, out, err);
	} 
	for (size_t i = 0; i < len; i++) {
		gu_out_utf8(src[i], out, err);
		if (!gu_ok(err)) {
			return i;
		}
	}
	return len;

}

#ifndef GU_CHAR_ASCII

void gu_str_out_utf8_(const char* str, GuOut* out, GuError* err)
{
	if (gu_out_is_buffered(out)) {
		size_t len = strlen(str);
		size_t sz;
		uint8_t* buf = gu_out_begin_span(out, &sz);
		GuPool* tmp_pool = NULL;
		if (sz < len) {
			gu_out_end_span(out, 0);
			buf = NULL;
		}
		if (buf == NULL) {
			tmp_pool = gu_pool_new();
			buf = gu_new_n(tmp_pool, uint8_t, len);
		}
		for (size_t i = 0; i < len; i++) {
			GuUCS ucs = gu_char_ucs(str[i]);
			buf[i] = (uint8_t) ucs;
		}
		if (tmp_pool) {
			gu_out_bytes(out, buf, len, err);
			gu_pool_free(tmp_pool);
		} else {
			gu_out_end_span(out, len);
		}
	} else {
		for (const char* p = str; *p; p++) {
			GuUCS ucs = gu_char_ucs(*p);
			gu_out_u8(out, (uint8_t) ucs, err);
			// defer error checking
		}
	}
}

#endif

extern inline void 
gu_str_out_utf8(const char* str, GuOut* out, GuError* err);

#if 0
static size_t
gu_utf8_writer_write(GuWriter* wtr, const GuUCS* ubuf,
		     size_t len, GuError* err)
{
	GuUTF8Writer* uwtr = (GuUTF8Writer*) wtr;

}

static void
gu_utf8_writer_flush(GuWriter* wtr, GuError* err)
{
	GuUTF8Writer* uwtr = (GuUTF8Writer*) wtr;
	gu_out_flush(uwtr->out, err);
}

GuWriter*
gu_utf8_writer(GuOut* out, GuPool* pool)
{
	return (GuWriter*) gu_new_s(
		pool, GuUTF8Writer,
		.wtr = { .write = gu_utf8_writer_write,
			 .flush = gu_utf8_writer_flush },
		.out = out);
}

#endif


typedef struct GuUTF8Reader GuUTF8Reader;

struct GuUTF8Reader {
	GuReader rdr;
	GuIn* in;
};

static size_t
gu_utf8_reader_read(GuReader* rdr, GuUCS* buf, size_t max_len, GuError* err)
{
	GuUTF8Reader* urdr = (GuUTF8Reader*) rdr;
	GuError* tmp_err = gu_error(err, GuEOF, NULL);
	size_t n = 0;
	while (n < max_len) {
		buf[n] = gu_in_utf8(urdr->in, tmp_err);
		if (!gu_ok(tmp_err)) {
			break;
		}
		n++;
	}
	return n;
}

GuReader*
gu_utf8_reader(GuIn* in, GuPool* pool)
{
	return (GuReader*) gu_new_s(pool, GuUTF8Reader,
				    .rdr = { gu_utf8_reader_read },
				    .in = in);
}


