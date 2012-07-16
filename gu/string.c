// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <gu/type.h>
#include <gu/out.h>
#include <gu/seq.h>
#include <gu/map.h>
#include <gu/string.h>
#include <gu/utf8.h>
#include <gu/assert.h>
#include "config.h"

const GuString gu_empty_string = { 1 };

const GuString gu_null_string = { (uintptr_t)(void*)NULL };

struct GuStringBuf {
	GuByteBuf* bbuf;
	GuWriter* wtr;
};

GuStringBuf*
gu_string_buf(GuPool* pool)
{
	GuBuf* buf = gu_new_buf(uint8_t, pool);
	GuOut* out = gu_buf_out(buf, pool);
	GuWriter* wtr = gu_new_utf8_writer(out, pool);
	return gu_new_s(pool, GuStringBuf,
			.bbuf = buf,
			.wtr = wtr);
}

GuWriter*
gu_string_buf_writer(GuStringBuf* sb)
{
	return sb->wtr;
}

GuString
gu_utf8_string(const uint8_t* buf, size_t sz, GuPool* pool)
{
	if (sz < GU_MIN(sizeof(GuWord), 128)) {
		GuWord w = 0;
		for (size_t n = 0; n < sz; n++) {
			w = w << 8 | buf[n];
		}
		w = w << 8 | (sz << 1) | 1;
		return (GuString) { w };
	}
	uint8_t* p = NULL;
	if (sz < 256) {
		p = gu_malloc_aligned(pool, 1 + sz, 2);
		p[0] = (uint8_t) sz;
	} else {
		uint8_t* p =
			gu_malloc_prefixed(pool, gu_alignof(size_t),
					   sizeof(size_t), 1, 1 + sizeof(sz));
		((size_t*) p)[-1] = sz;
		p[0] = 0;
	}
	memcpy(&p[1], buf, sz);
	return (GuString) { (GuWord) (void*) p };
}



GuString
gu_string_buf_freeze(GuStringBuf* sb, GuPool* pool)
{
	gu_writer_flush(sb->wtr, gu_null_exn());
	uint8_t* data = gu_buf_data(sb->bbuf);
	size_t len = gu_buf_length(sb->bbuf);
	return gu_utf8_string(data, len, pool);
}

static bool
gu_string_is_long(GuString s) 
{
	return !(s.w_ & 1);
}

bool
gu_string_is_stable(GuString s)
{
	return !gu_string_is_long(s);
}

static size_t
gu_string_long_size(GuString s)
{
	gu_assert(gu_string_is_long(s));
	uint8_t* p = (void*) s.w_;
	uint8_t sz = p[0];
	if (sz > 0) {
		return sz;
	}
	return ((size_t*) p)[-1];
}

static size_t
gu_string_short_size(GuString s)
{
	gu_assert(!gu_string_is_long(s));
	return (s.w_ & 0xff) >> 1;
}

static size_t
gu_string_size(GuString s)
{
	if (gu_string_is_long(s)) {
		return gu_string_long_size(s);
	} else {
		return gu_string_short_size(s);
	}
}

static uint8_t*
gu_string_long_data(GuString s)
{
	gu_require(gu_string_is_long(s));
	uint8_t* p = (void*) s.w_;
	return &p[1];
}

GuString
gu_string_copy(GuString string, GuPool* pool)
{
	if (gu_string_is_long(string)) {
		uint8_t* data = gu_string_long_data(string);
		size_t sz = gu_string_long_size(string);
		return gu_utf8_string(data, sz, pool);
	} else {
		return string;
	}
}

static void
gu_string_short_write_data(GuString s, uint8_t* buf)
{
	gu_require(!gu_string_is_long(s));
	GuWord w = s.w_;
	size_t sz = (w & 0xff) >> 1;
	for (int i = sz - 1; i >= 0; i--) {
		w >>= 8;
		buf[i] = w & 0xff;
	}
}

static void
gu_string_write_data(GuString s, uint8_t* buf)
{
	if (gu_string_is_long(s)) {
		size_t sz = gu_string_long_size(s);
		uint8_t* data = gu_string_long_data(s);
		memcpy(buf, data, sz);
	} else {
		gu_string_short_write_data(s, buf);
	}
}


GuBytes
gu_string_utf8(GuString s, GuPool* pool)
{
	size_t sz = gu_string_size(s);
	GuBytes bytes = gu_new_seq(uint8_t, sz, pool);
	uint8_t* data = gu_seq_data(bytes);
	gu_string_write_data(s, data);
	return bytes;
}


void
gu_string_write(GuString s, GuWriter* wtr, GuExn* err)
{
	uint8_t buf[sizeof(GuWord)];
	uint8_t* src;
	size_t sz = gu_string_size(s);
	if (gu_string_is_long(s)) {
		src = gu_string_long_data(s);
	} else {
		gu_assert(sz <= sizeof(GuWord));
		gu_string_write_data(s, buf);
		src = buf;
	}
	gu_utf8_write(src, sz, wtr, err);
}

GuReader*
gu_string_reader(GuString s, GuPool* pool)
{
	GuCSlice buf;
	if (gu_string_is_long(s)) {
		buf.sz = gu_string_long_size(s);
		buf.p = gu_string_long_data(s);
	} else {
		buf.sz = gu_string_short_size(s);
		uint8_t* p = gu_new_n(uint8_t, buf.sz, pool);
		buf.p = p;
		gu_string_short_write_data(s, p);
	}
	GuIn* in = gu_data_in(buf, pool);
	GuReader* rdr = gu_new_utf8_reader(in, pool);
	return rdr;
}


GuString
gu_format_string_v(const char* fmt, va_list args, GuPool* pool)
{
	GuPool* tmp_pool = gu_local_pool();
	GuStringBuf* sb = gu_string_buf(tmp_pool);
	GuWriter* wtr = gu_string_buf_writer(sb);
	GuExn* exn = gu_null_exn();
	gu_vprintf(fmt, args, wtr, exn);
	gu_writer_flush(wtr, exn);
	GuString s = gu_string_buf_freeze(sb, pool);
	gu_pool_free(tmp_pool);
	return s;
}

GuString
gu_format_string(GuPool* pool, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	GuString s = gu_format_string_v(fmt, args, pool);
	va_end(args);
	return s;
}

GuString
gu_str_string(const char* str, GuPool* pool)
{
#ifdef GU_CHAR_ASCII
	return gu_utf8_string((const uint8_t*) str, strlen(str), pool);
#else
	GuPool* tmp_pool = gu_local_pool();
	GuStringBuf* sb = gu_string_buf(tmp_pool);
	GuWriter* wtr = gu_string_buf_writer(sb);
	GuExn* exn = gu_null_exn();
	gu_puts(str, wtr, exn);
	gu_writer_flush(wtr, exn);
	GuString s = gu_string_buf_freeze(sb, pool);
	gu_pool_free(tmp_pool);
	return s;
#endif
}

bool
gu_string_is_null(GuString s)
{
	return (s.w_ == (uintptr_t)(void*)NULL);
}

GuWord
gu_string_hash(GuString s)
{
	if (s.w_ & 1) {
		return s.w_;
	}
	size_t sz = gu_string_size(s);
	uint8_t* data = gu_string_long_data(s);
	return gu_hash_bytes(0, data, sz);
}

bool
gu_string_eq(GuString s1, GuString s2)
{
	if (s1.w_ == s2.w_) {
		return true;
	} else if (gu_string_is_long(s1) && gu_string_is_long(s2)) {
		size_t sz1 = gu_string_long_size(s1);
		size_t sz2 = gu_string_long_size(s2);
		if (sz1 != sz2) {
			return false;
		}
		uint8_t* data1 = gu_string_long_data(s1);
		uint8_t* data2 = gu_string_long_data(s2);
		return (memcmp(data1, data2, sz1) == 0);
	}
	return false;

}


static GuHash
gu_string_hasher_hash(GuHasher* self, GuHash h, const void* p)
{
	const GuString* sp = p;
	return gu_string_hash(*sp);
}

static bool
gu_string_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	const GuString* sp1 = p1;
	const GuString* sp2 = p2;
	return gu_string_eq(*sp1, *sp2);
}

GuHasher gu_string_hasher[1] = {
	{
		.eq = { gu_string_eq_fn },
		.hash = gu_string_hasher_hash
	}
};


GU_DEFINE_TYPE(GuString, GuOpaque, _);
GU_DEFINE_TYPE(GuStrings, GuSeq, gu_type(GuString));
GU_DEFINE_KIND(GuStringMap, GuMap);
