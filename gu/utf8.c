#include <gu/assert.h>
#include <gu/utf8.h>
#include <guconfig.h>

static size_t
gu_utf8_decode_one_unsafe(const uint8_t* src, GuUCS* dst)
{
	uint8_t c = src[0];
	if (c < 0x80) {
		*dst = c;
		return 1;
	}
	size_t len = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
	const uint32_t mask = 0x070f1f7f;
	GuUCS u = c & (mask >> (len * 8));
	size_t i;
	for (i = 1; i <= len; i++) {
		c = src[i];
		u = u << 6 | (c & 0x3f);
	}
	*dst = u;
	return i;
}

static size_t
gu_utf8_decode_one(const uint8_t* src, GuUCS* dst, GuExn* err)
{
	uint8_t c = src[0];
	if (c < 0x80) {
		*dst = c;
		return 1;
	} else if (c < 0xc2 || c > 0xf4) {
		goto fail;
	}
	size_t len = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
	const uint32_t mask = 0x070f1f7f;
	GuUCS u = c & (mask >> (len * 8));
	size_t i;
	for (i = 1; i <= len; i++) {
		c = src[i];
		u = u << 6 | (c & 0x3f);
	}
	if (!gu_ucs_valid(u)) {
		goto fail;
	}
	*dst = u;
	return i;
fail:
	gu_raise(err, GuUCSExn);
	return 0;
}


void
gu_utf8_decode_unsafe(const uint8_t** src_inout, const uint8_t* src_end,
		      GuUCS** dst_inout, GuUCS* dst_end)
{
	const uint8_t* src_curr = *src_inout;
	size_t src_rem = src_end - src_curr;
	GuUCS* dst_curr = *dst_inout;
	size_t dst_rem = dst_end - dst_curr;

	while (true) {
		size_t sz = GU_MIN((src_rem + 3) / 4, dst_rem);
		if (!sz) break;
		for (size_t i = 0; i < sz; i++) {
			size_t len = gu_utf8_decode_one_unsafe(src_curr,
							       &dst_curr[i]);
			src_curr += len;
		}
		src_rem = src_end - src_curr;
		dst_curr += sz;
		dst_rem -= sz;
	}
	// We assume the input is well-formed and doesn't end in
	// a partial character.
	gu_assert(src_curr <= src_end);
	*src_inout = src_curr;
	*dst_inout = dst_curr;
}

static size_t
gu_utf8_encode_length(GuUCS ucs)
{
	return (GU_LIKELY(ucs < 0x80) ? 1 :
		ucs < 0x800 ? 2 :
		ucs < 0x10000 ? 3 :
		4);
}

static size_t
gu_utf8_encode_one_unsafe(GuUCS ucs, uint8_t* buf)
{
	gu_require(gu_ucs_valid(ucs));
	if (GU_LIKELY(ucs < 0x80)) {
		buf[0] = (uint8_t) ucs;
		return 1;
	}
	size_t n = ucs < 0x800 ? 1 : ucs < 0x10000 ? 2 : 3;
	for (size_t i = n; i > 0; i--) {
		buf[i] = 0x80 | (uint8_t)(ucs & 0x3f);
		ucs >>= 6;
	}
	buf[0] = (uint8_t) (ucs | ~(0x7f >> n));
	return n + 1;
}

void
gu_utf8_encode(const GuUCS** src_inout, const GuUCS* src_end,
	       uint8_t** dst_inout, uint8_t* dst_end)
{
	const GuUCS* src_curr = *src_inout;
	uint8_t* dst_curr = *dst_inout;

	while (true) {
		size_t n = GU_MIN((src_end - src_curr),
				  (dst_end - dst_curr) / 4);
		if (!n) break;
		for (size_t i = 0; i < n; i++) {
			size_t len = gu_utf8_encode_one_unsafe(src_curr[i],
							       dst_curr);
			dst_curr += len;
		}
		src_curr += n;
	}
	while (src_curr < src_end && dst_curr < dst_end) {
		GuUCS ucs = *src_curr;
		size_t len = gu_utf8_encode_length(ucs);
		if (len > (size_t)(dst_end - dst_curr)) {
			break;
		}
		gu_utf8_encode_one_unsafe(ucs, dst_curr);
		dst_curr += len;
		src_curr++;
	}
	*src_inout = src_curr;
	*dst_inout = dst_curr;
}




GuUCS
gu_in_utf8_(GuIn* in, GuExn* err)
{
	uint8_t c = gu_in_u8(in, err);
	if (!gu_ok(err)) return 0;
	size_t len = c < 0xc0 ? 0 : c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
	uint8_t buf[4];
	buf[0] = c;
	size_t got = gu_in_some(in, &buf[1], len, len, err);
	if (got < len) {
		if (gu_ok(err)) {
			// If reading the extra bytes causes EOF, it is an
			// encoding error, not a legitimate end of
			// character stream.
			gu_raise(err, GuUCSExn);
		}
		return 0;
	}
	GuUCS ret = 0;
	gu_utf8_decode_one(buf, &ret, err);
	return ret;
}

char
gu_in_utf8_char_(GuIn* in, GuExn* err)
{
	GuUCS u = gu_in_utf8(in, err);
	if (!gu_ok(err)) return 0;
	char c = gu_ucs_char(u);
	if (c == '\0' && u != 0) {
		gu_raise(err, GuUCSExn);
	}
	return c;
}

void
gu_out_utf8_long_(GuUCS ucs, GuOut* out, GuExn* err)
{
	uint8_t buf[4];
	size_t req = 4;
	uint8_t* span = gu_out_begin_span(out, buf, &req, err);
	if (!gu_ok(err)) return;
	size_t sz = gu_utf8_encode_one_unsafe(ucs, span);
	gu_out_end_span(out, span, sz, err);
}

extern inline void
gu_out_utf8(GuUCS ucs, GuOut* out, GuExn* err);


size_t
gu_utf32_out_utf8(const GuUCS* src, size_t len, GuOut* out, GuExn* err)
{
	const GuUCS* src_curr = src;
	const GuUCS* src_end = &src[len];
	while (src_curr < src_end) {
		const GuUCS* src_tmp = src_curr;
		uint8_t buf[4];
		size_t dst_sz = 4;
		uint8_t* dst = gu_out_begin_span(out, buf, &dst_sz, err);
		if (!gu_ok(err)) break;
		uint8_t* dst_curr = dst;
		gu_utf8_encode(&src_tmp, src_end, &dst_curr, &dst[dst_sz]);
		gu_out_end_span(out, dst, dst_curr - dst, err);
		if (!gu_ok(err)) break;
		src_curr = src_tmp;
	}
	return src_curr - src;
}

#ifndef GU_CHAR_ASCII

void
gu_str_out_utf8_(const char* str, GuOut* out, GuExn* err)
{
	size_t len = strlen(str);
	size_t i = 0;
	while (i < len) {
		size_t sz;
		uint8_t* buf = gu_out_begin_span(out, 1, &sz, err);
		if (!gu_ok(err)) return;
		size_t n = GU_MIN(sz, len - i);
		for (size_t j = 0; j < n; j++) {
			GuUCS ucs = gu_char_ucs(str[i + j]);
			gu_assert(ucs < 0x80);
			buf[j] = (uint8_t) ucs;
		}
		gu_out_end_span(out, n);
		i += n;
	}
}

#endif

extern inline void 
gu_str_out_utf8(const char* str, GuOut* out, GuExn* err);

extern inline GuUCS
gu_in_utf8(GuIn* in, GuExn* err);

extern inline char
gu_in_utf8_char(GuIn* in, GuExn* err);
