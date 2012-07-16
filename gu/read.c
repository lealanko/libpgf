// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <gu/read.h>

extern inline GuUCS
gu_read_ucs(GuReader* rdr, GuExn* err);

extern inline char
gu_getc(GuReader* rdr, GuExn* err);

GuReader*
gu_new_utf8_reader(GuIn* utf8_in, GuPool* pool)
{
	GuReader* rdr = gu_new(GuReader, pool);
	rdr->in_ = gu_init_in(gu_in_proxy_stream(utf8_in, pool));
	return rdr;
}



enum {
	BUF_SIZE = 1024
};

typedef struct GuLocaleInStream GuLocaleInStream;

struct GuLocaleInStream {
	GuInStream stream;
	GuIn* in;
	mbstate_t ps;
	bool failed;
	uint8_t* utf8_frag_cur;
	uint8_t* utf8_frag_end;
	uint8_t utf8_frag[4];
};

static size_t
gu_locale_reader_input(GuInStream* self, GuSlice utf8_buf, GuExn* err)
{
	GuLocaleInStream* lis = (GuLocaleInStream*) self;
	uint8_t* dst_cur = utf8_buf.p;
	uint8_t* dst_end = &dst_cur[utf8_buf.sz];
	size_t frag_len = lis->utf8_frag_end - lis->utf8_frag_cur;
	if (frag_len) {
		size_t clen = GU_MIN(frag_len, utf8_buf.sz);
		memcpy(dst_cur, lis->utf8_frag_cur, clen);
		lis->utf8_frag_cur += clen;
		return clen;
	}
	if (lis->failed) {
		gu_raise(err, GuUCSExn);
		return 0;
	}
read_again:;
	GuUCS ucs_buf[BUF_SIZE];
	size_t mbs_max = (dst_end - dst_cur + 3) / 4;
	uint8_t fallback[BUF_SIZE];
	GuSlice req = { fallback, GU_MIN(mbs_max, BUF_SIZE) };
	GuCSlice mbs = gu_in_begin_span(lis->in, req, err);
	if (!gu_ok(err)) return 0;
	size_t mbs_len = GU_MIN(mbs.sz, mbs_max);
	const char* mbs_buf = (const char*) mbs.p;
	size_t mbs_cur = 0;
	size_t ucs_len = 0;
	while (mbs_cur < mbs_len) {
		// This is slow, but mbsrtowcs would require
		// complete zero-terminated strings.
		size_t ret = mbrtowc(&ucs_buf[ucs_len], &mbs_buf[mbs_cur],
				     mbs_len - mbs_cur, &lis->ps);
		if (ret == (size_t) -2) {
			if (ucs_len == 0) {
				// didn't even get a single complete character
				gu_in_end_span(lis->in, mbs_len);
				goto read_again;
			}
			mbs_cur = mbs_len;
			break;
		} else if (ret == (size_t) -1) {
			if (ucs_len == 0) {
				gu_raise(err, GuUCSExn);
				gu_in_end_span(lis->in, mbs_len);
				return 0;
			}
			// We already got some input before the decoding error,
			// so process that and return failure next time.
			lis->failed = true;
			break;
		} else if (ret == 0) {
			// Decoded L'\0'. The actual number of bytes consumed
			// is unknown, but probably cannot be other than 1?
			ret = 1;
		}
		mbs_cur += ret;
		ucs_len++;
	}
	gu_in_end_span(lis->in, mbs_len);
	const GuUCS* ucs_cur = ucs_buf;
	const GuUCS* ucs_end = &ucs_buf[ucs_len];
	gu_utf8_encode(&ucs_cur, ucs_end, &dst_cur, dst_end);
	if (ucs_cur < ucs_end) {
		gu_assert(ucs_cur == &ucs_end[-1]);
		size_t dst_rem = dst_end - dst_cur;
		gu_assert(dst_rem < 4);
		lis->utf8_frag_end = lis->utf8_frag;
		gu_utf8_encode(&ucs_cur, ucs_end, &lis->utf8_frag_end,
			       &lis->utf8_frag[4]);
		gu_assert(ucs_cur == ucs_end);
		gu_assert((size_t) (lis->utf8_frag_end - lis->utf8_frag)
			  > dst_rem);
		memcpy(dst_cur, lis->utf8_frag, dst_rem);
		dst_cur += dst_rem;
		lis->utf8_frag_cur = &lis->utf8_frag[dst_rem];
	}
	return dst_cur - utf8_buf.p;
}

GuReader*
gu_new_locale_reader(GuIn* locale_in, GuPool* pool)
{
	GuLocaleInStream* lis = gu_new_i(pool, GuLocaleInStream,
					 .stream.input = gu_locale_reader_input,
					 .in = locale_in,
					 .ps = { 0 },
					 .failed = false);
	lis->utf8_frag_cur = lis->utf8_frag_end = lis->utf8_frag;
	return gu_new_i(pool, GuReader, .in_ = gu_init_in(&lis->stream));
}
