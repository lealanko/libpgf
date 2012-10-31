// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef GU_OUT_H_
#define GU_OUT_H_

#include <gu/defs.h>
#include <gu/assert.h>
#include <gu/exn.h>

typedef struct GuOut GuOut;

typedef struct GuOutStream GuOutStream;
typedef struct GuOutStreamFuns GuOutStreamFuns;

struct GuOutStreamFuns {
	GuSlice (*begin_buf)(GuOutStream* self, size_t req, GuExn* err);
	void (*end_buf)(GuOutStream* self, size_t span, GuExn* err);
	size_t (*output)(GuOutStream* self, GuCSlice buf, GuExn* err);
	void (*flush)(GuOutStream* self, GuExn* err);
};

struct GuOutStream {
	GuOutStreamFuns* funs;
};

struct GuOut {
	uint8_t* restrict buf_end;
	ptrdiff_t buf_curr;
	size_t buf_size;
	GuOutStream* stream;
	GuFinalizer fini;
};


GuOut
gu_init_out(GuOutStream* stream);

GuOut*
gu_new_out(GuOutStream* stream, GuPool* pool);

inline bool
gu_out_is_buffered(GuOut* out)
{
	return !!out->stream->funs->begin_buf;
}

GuOutStream*
gu_out_proxy_stream(GuOut* out, GuPool* pool);

GuOut
gu_init_buffered_out(GuOut* out, size_t buf_sz, GuPool* pool);

GuOut*
gu_new_buffered_out(GuOut* out, size_t buf_sz, GuPool* pool);

GuOut*
gu_out_buffered(GuOut* out, GuPool* pool);

GuSlice
gu_out_begin_span(GuOut* out, GuSlice fallback, GuExn* err);

void
gu_out_end_span(GuOut* out, GuSlice span, GuExn* err);

size_t
gu_out_bytes_(GuOut* restrict out, GuCSlice src, GuExn* err);

inline bool
gu_out_try_buf_(GuOut* restrict out, GuCSlice src)
{
	gu_require(src.sz <= PTRDIFF_MAX);
	ptrdiff_t curr = out->buf_curr;
	ptrdiff_t new_curr = curr + (ptrdiff_t) src.sz;
	if (GU_UNLIKELY(new_curr > 0)) {
		return false;
	}
	memcpy(&out->buf_end[curr], src.p, src.sz);
	out->buf_curr = new_curr;
	return true;
}

inline size_t
gu_out_bytes(GuOut* restrict out, GuCSlice src, GuExn* err)
{
	if (GU_LIKELY(gu_out_try_buf_(out, src))) {
		return src.sz;
	}
	return gu_out_bytes_(out, src, err);
}

void
gu_out_flush(GuOut* out, GuExn* err);

inline bool
gu_out_try_u8_(GuOut* restrict out, uint8_t u)
{
	ptrdiff_t curr = out->buf_curr;
	ptrdiff_t new_curr = curr + 1;
	if (GU_UNLIKELY(new_curr > 0)) {
		return false;
	}
	out->buf_end[curr] = u;
	out->buf_curr = new_curr;
	return true;
}

inline void
gu_out_u8(GuOut* restrict out, uint8_t u, GuExn* err)
{
	if (GU_UNLIKELY(!gu_out_try_u8_(out, u))) {
		extern void gu_out_u8_(GuOut* restrict out, uint8_t u, 
				       GuExn* err);
		gu_out_u8_(out, u, err);
	}
}

inline void
gu_out_s8(GuOut* restrict out, int8_t i, GuExn* err)
{
	gu_out_u8(out, (uint8_t) i, err);
}



void
gu_out_u16le(GuOut* out, uint16_t u, GuExn* err);

void
gu_out_u16be(GuOut* out, uint16_t u, GuExn* err);

void
gu_out_s16le(GuOut* out, int16_t u, GuExn* err);

void
gu_out_s16be(GuOut* out, int16_t u, GuExn* err);

void
gu_out_u32le(GuOut* out, uint32_t u, GuExn* err);

void
gu_out_u32be(GuOut* out, uint32_t u, GuExn* err);

void
gu_out_s32le(GuOut* out, int32_t u, GuExn* err);

void
gu_out_s32be(GuOut* out, int32_t u, GuExn* err);

void
gu_out_u64le(GuOut* out, uint64_t u, GuExn* err);

void
gu_out_u64be(GuOut* out, uint64_t u, GuExn* err);

void
gu_out_s64le(GuOut* out, int64_t u, GuExn* err);

void
gu_out_s64be(GuOut* out, int64_t u, GuExn* err);

void
gu_out_f64le(GuOut* out, double d, GuExn* err);

void
gu_out_f64be(GuOut* out, double d, GuExn* err);

#endif // GU_OUT_H_
