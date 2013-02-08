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

typedef uint8_t GuShortData[sizeof(GuWord)];

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
gu_utf8_string(GuCSlice buf, GuPool* pool)
{
	if (buf.sz < GU_MIN(sizeof(GuWord), 128)) {
		GuWord w = 0;
		for (size_t n = 0; n < buf.sz; n++) {
			w = w << 8 | buf.p[n];
		}
		w = w << 8 | (buf.sz << 1) | 1;
		return (GuString) { w };
	}
	uint8_t* p = NULL;
	if (buf.sz < 256) {
		p = gu_malloc_aligned(pool, 1 + buf.sz, 2);
		p[0] = (uint8_t) buf.sz;
	} else {
		uint8_t* p =
			gu_malloc_prefixed(pool,
					   gu_alignof(size_t), sizeof(size_t),
					   1, 1 + buf.sz);
		((size_t*) p)[-1] = buf.sz;
		p[0] = 0;
	}
	memcpy(&p[1], buf.p, buf.sz);
	return (GuString) { (GuWord) (void*) p };
}


GuString
gu_string_buf_freeze(GuStringBuf* sb, GuPool* pool)
{
	gu_writer_flush(sb->wtr, gu_null_exn());
	uint8_t* data = gu_buf_data(sb->bbuf);
	size_t len = gu_buf_length(sb->bbuf);
	return gu_utf8_string(gu_cslice(data, len), pool);
}

bool
gu_string_is_stable(GuString s)
{
	return ((s.w_ & 1) == 1);
}

static GuCSlice
gu_string_open(GuString s, GuShortData* short_data)
{
	GuCSlice ret;
	GuWord w = s.w_;
	if (!(w & 1)) {
		// long string
		uint8_t* p = (void*) w;
		ret.p = &p[1];
		ret.sz = p[0];
		if (!ret.sz) {
			ret.sz = ((size_t*) p)[-1];
		}
	} else {
		ret.sz = (w & 0xff) >> 1;
		uint8_t* p = *short_data;
		ret.p = p;
		if (p) {
			for (int i = ret.sz - 1; i >= 0; i--) {
				w >>= 8;
				p[i] = w & 0xff;
			}
		}
	}
	return ret;
}


GuString
gu_string_copy(GuString string, GuPool* pool)
{
	if (gu_string_is_stable(string)) {
		return string;
	}
	GuCSlice data = gu_string_open(string, NULL);
	return gu_utf8_string(data, pool);
}


GuSlice
gu_string_utf8(GuString s, GuPool* pool)
{
	GuShortData short_data;
	GuCSlice data = gu_string_open(s, &short_data);
	GuSlice ret = gu_malloc_slice(pool, data.sz);
	memcpy(ret.p, data.p, data.sz);
	return ret;
}

void
gu_string_write(GuString s, GuWriter* wtr, GuExn* err)
{
	GuShortData short_data;
	GuCSlice data = gu_string_open(s, &short_data);
	gu_utf8_write(data, wtr, err);
}

GuReader*
gu_string_reader(GuString s, GuPool* pool)
{
	GuShortData short_data;
	GuCSlice buf = gu_string_open(s, &short_data);
	if (buf.p == short_data) {
		uint8_t* p = gu_new_n(uint8_t, buf.sz, pool);
		memcpy(p, short_data, buf.sz);
		buf.p = p;
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
	return gu_utf8_string(gu_str_cslice(str), pool);
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
	GuCSlice data = gu_string_open(s, NULL);
	return gu_hash_bytes(0, data.p, data.sz);
}

bool
gu_string_eq(GuString s1, GuString s2)
{
	if (s1.w_ == s2.w_) {
		return true;
	}
	GuShortData short1, short2;
	GuCSlice data1 = gu_string_open(s1, &short1);
	GuCSlice data2 = gu_string_open(s2, &short2);
	return gu_cslice_eq(data1, data2);
}


static GuHash
gu_string_hasher_hash(GuHasher* self, GuHash h, const void* p)
{
	const GuString* sp = p;
	return gu_string_hash(*sp);
}

static bool
gu_string_eq_fn(GuEq* self, const void* p1, const void* p2)
{
	const GuString* sp1 = p1;
	const GuString* sp2 = p2;
	return gu_string_eq(*sp1, *sp2);
}

GU_DEFINE_HASHER(gu_string_hasher, gu_string_hasher_hash, gu_string_eq_fn);


GU_DEFINE_TYPE(GuString, GuOpaque, _);
GU_DEFINE_TYPE(GuStrings, GuSeq, gu_type(GuString));
GU_DEFINE_KIND(GuStringMap, GuMap);
