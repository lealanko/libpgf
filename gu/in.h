// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef GU_IN_H_
#define GU_IN_H_

#include <gu/defs.h>
#include <gu/exn.h>
#include <gu/assert.h>

typedef struct GuInStream GuInStream;
typedef struct GuInStreamFuns GuInStreamFuns;

struct GuInStreamFuns {
	GuCSlice (*begin_buffer)(GuInStream* self, GuExn* err);
	void (*end_buffer)(GuInStream* self, size_t consumed, GuExn* err);
	size_t (*input)(GuInStream* self, GuSlice buf, GuExn* err);
};

struct GuInStream {
	GuInStreamFuns* funs;
};

typedef struct GuIn GuIn;

struct GuIn {
	const uint8_t* restrict buf_end;
	ptrdiff_t buf_curr;
	size_t buf_size;
	GuInStream* stream;
	size_t stream_pos;
	GuFinalizer fini;
};


GuIn
gu_init_in(GuInStream* stream);

GuIn*
gu_new_in(GuInStream* stream, GuPool* pool);

GuInStream*
gu_in_proxy_stream(GuIn* in, GuPool* pool);

GuCSlice
gu_in_begin_span(GuIn* in, GuSlice req, GuExn* err);

void
gu_in_end_span(GuIn* in, size_t consumed);

size_t
gu_in_tell(GuIn* in);

size_t
gu_in_some(GuIn* in, GuSlice buf, size_t min_len, GuExn* err);

inline void
gu_in_bytes(GuIn* in, GuSlice buf, GuExn* err)
{
	gu_require(buf.sz < PTRDIFF_MAX);
	ptrdiff_t curr = in->buf_curr;
	ptrdiff_t new_curr = curr + (ptrdiff_t) buf.sz;
	if (GU_UNLIKELY(new_curr > 0)) {
		extern void gu_in_bytes_(GuIn* in, GuSlice buf, GuExn* err);
		gu_in_bytes_(in, buf, err);
		return;
	}
	memcpy(buf.p, &in->buf_end[curr], buf.sz);
	in->buf_curr = new_curr;
}

inline int
gu_in_peek_u8(GuIn* restrict in)
{
	if (GU_UNLIKELY(in->buf_curr == 0)) {
		return -1;
	}
	return in->buf_end[in->buf_curr];
}

inline void
gu_in_consume(GuIn* restrict in, size_t sz)
{
	gu_require((ptrdiff_t) sz + in->buf_curr <= 0);
	in->buf_curr += sz;
}


inline uint8_t 
gu_in_u8(GuIn* restrict in, GuExn* err)
{
	if (GU_UNLIKELY(in->buf_curr == 0)) {
		extern uint8_t gu_in_u8_(GuIn* restrict in, GuExn* err);
		return gu_in_u8_(in, err);
	}
	return in->buf_end[in->buf_curr++];
}

int8_t 
gu_in_s8(GuIn* in, GuExn* err);

uint16_t
gu_in_u16le(GuIn* in, GuExn* err);

uint16_t
gu_in_u16be(GuIn* in, GuExn* err);

int16_t
gu_in_s16le(GuIn* in, GuExn* err);

int16_t
gu_in_s16be(GuIn* in, GuExn* err);

uint32_t
gu_in_u32le(GuIn* in, GuExn* err);

uint32_t
gu_in_u32be(GuIn* in, GuExn* err);

int32_t
gu_in_s32le(GuIn* in, GuExn* err);

int32_t
gu_in_s32be(GuIn* in, GuExn* err);

uint64_t
gu_in_u64le(GuIn* in, GuExn* err);

uint64_t
gu_in_u64be(GuIn* in, GuExn* err);

int64_t
gu_in_s64le(GuIn* in, GuExn* err);

int64_t
gu_in_s64be(GuIn* in, GuExn* err);

double
gu_in_f64le(GuIn* in, GuExn* err);

double
gu_in_f64be(GuIn* in, GuExn* err);

GuIn*
gu_new_buffered_in(GuIn* in, size_t sz, GuPool* pool);

GuIn*
gu_data_in(GuCSlice buf, GuPool* pool);


extern GU_DECLARE_TYPE(GuEOF, abstract);

#include <gu/type.h>

#endif // GU_IN_H_
