#ifndef GU_OUT_H_
#define GU_OUT_H_

#include <gu/defs.h>
#include <gu/assert.h>
#include <gu/error.h>

typedef struct GuOut GuOut;

typedef struct GuOutBuffer GuOutBuffer;

struct GuOutBuffer {
	uint8_t* (*begin)(GuOutBuffer* self, size_t* sz_out);
	void (*end)(GuOutBuffer* self, size_t size, GuError* err);
};


struct GuOut {
	uint8_t* restrict buf_end;
	ptrdiff_t buf_curr;
	uint8_t* buf_start;
	GuOutBuffer* buffer;
	size_t (*output)(GuOut* self, const uint8_t* buf, size_t size,
			 GuError* err);
	void (*flush)(GuOut* self, GuError* err);
};

inline bool
gu_out_is_buffered(GuOut* out)
{
	return (out->buffer != NULL);
}

uint8_t*
gu_out_begin_span(GuOut* out, size_t* sz_inout);

void
gu_out_end_span(GuOut* out, size_t sz);

GuOutBuffer*
gu_out_make_buffer(GuOut* out, size_t sz, GuPool* pool);

void
gu_out_set_buffer(GuOut* out, GuOutBuffer* buffer, GuError* err);

void
gu_out_enable_buffering(GuOut* out, size_t buf_sz, GuPool* pool);

size_t
gu_out_bytes_(GuOut* restrict out, const uint8_t* restrict src, 
	      size_t len, GuError* err);

inline bool
gu_out_try_buf_(GuOut* restrict out, const uint8_t* restrict src, size_t len)
{
	gu_require(len <= PTRDIFF_MAX);
	ptrdiff_t curr = out->buf_curr;
	ptrdiff_t new_curr = curr + (ptrdiff_t) len;
	if (GU_UNLIKELY(new_curr > 0)) {
		return false;
	}
	memcpy(&out->buf_end[curr], src, len);
	out->buf_curr = new_curr;
	return true;
}

inline size_t
gu_out_bytes(GuOut* restrict out, const uint8_t* restrict src, size_t len, 
	     GuError* err)
{
	if (GU_LIKELY(gu_out_try_buf_(out, src, len))) {
		return len;
	}
	return gu_out_bytes_(out, src, len, err);
}

void
gu_out_flush(GuOut* out, GuError* err);

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
gu_out_u8(GuOut* restrict out, uint8_t u, GuError* err)
{
	if (GU_UNLIKELY(!gu_out_try_u8_(out, u))) {
		gu_out_bytes_(out, &u, 1, err);
	}
}

inline void
gu_out_s8(GuOut* restrict out, int8_t i, GuError* err)
{
	gu_out_u8(out, (uint8_t) i, err);
}



void
gu_out_u16le(GuOut* out, uint16_t u, GuError* err);

void
gu_out_u16be(GuOut* out, uint16_t u, GuError* err);

void
gu_out_s16le(GuOut* out, int16_t u, GuError* err);

void
gu_out_s16be(GuOut* out, int16_t u, GuError* err);

void
gu_out_u32le(GuOut* out, uint32_t u, GuError* err);

void
gu_out_u32be(GuOut* out, uint32_t u, GuError* err);

void
gu_out_s32le(GuOut* out, int32_t u, GuError* err);

void
gu_out_s32be(GuOut* out, int32_t u, GuError* err);

void
gu_out_u64le(GuOut* out, uint64_t u, GuError* err);

void
gu_out_u64be(GuOut* out, uint64_t u, GuError* err);

void
gu_out_s64le(GuOut* out, int64_t u, GuError* err);

void
gu_out_s64be(GuOut* out, int64_t u, GuError* err);

void
gu_out_f64le(GuOut* out, double d, GuError* err);

void
gu_out_f64be(GuOut* out, double d, GuError* err);

#endif // GU_OUT_H_
