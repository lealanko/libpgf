#include <gu/assert.h>
#include <gu/utf8.h>
#include "config.h"

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

static void
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

void
gu_out_utf8(GuUCS ucs, GuOut* out, GuError* err)
{
	gu_require(gu_ucs_valid(ucs));
	if (GU_LIKELY(ucs < 0x80)) {
		gu_out_u8(out, ucs, err);
	} else {
		gu_out_utf8_long_(ucs, out, err);
	}
}

typedef struct GuUTF8Writer GuUTF8Writer;

struct GuUTF8Writer {
	GuWriter wtr;
	GuOut* out;
};

static size_t
gu_utf8_writer_write(GuWriter* wtr, const GuUCS* ubuf,
		     size_t len, GuError* err)
{
	GuUTF8Writer* uwtr = (GuUTF8Writer*) wtr;
	const GuUCS* src = ubuf;
	const GuUCS* src_end = &ubuf[len];
	size_t done = 0;
	uint8_t buf[1024];
	uint8_t* end = &buf[1021];
	do {
		uint8_t* p = buf;
		do {
			size_t safe = (end - p) / 4;
			size_t count = GU_MIN(safe, (size_t) (src_end - src));
			for (size_t i = 0; i < count; i++) {
				GuUCS ucs = *src++;
				gu_require(gu_ucs_valid(ucs));
				p += gu_advance_utf8(ucs, p);
			}
		} while (src < src_end && end - p > 3);
		done += gu_out_bytes(uwtr->out, buf, p - buf, err);
	} while (done < len && gu_ok(err));
	return done;
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


