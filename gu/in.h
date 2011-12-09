#ifndef GU_IN_H_
#define GU_IN_H_

#include <gu/defs.h>
#include <gu/error.h>
#include <gu/assert.h>

typedef struct GuInStream GuInStream;

struct GuInStream {
	const uint8_t* (*next_buffer)(GuInStream* self, size_t* sz_out, 
				      GuError* err);
	size_t (*input)(GuInStream* self, uint8_t* buf, size_t max_sz, 
			GuError* err);
};

typedef struct GuIn GuIn;

struct GuIn {
	const uint8_t* restrict buf_end;
	ptrdiff_t buf_curr;
	GuInStream* stream;
};



inline GuIn
gu_init_in(GuInStream* stream)
{
	return (GuIn) { .buf_curr = 0,
			.buf_end = NULL,
			.stream = stream };
}

GuIn*
gu_make_in(GuInStream* stream, GuPool* pool);

size_t
gu_in_some(GuIn* in, uint8_t* buf, size_t max_len, GuError* err);

inline void
gu_in_bytes(GuIn* in, uint8_t* buf, size_t sz, GuError* err)
{
	gu_require(sz < PTRDIFF_MAX);
	ptrdiff_t curr = in->buf_curr;
	ptrdiff_t new_curr = curr + (ptrdiff_t) sz;
	if (GU_UNLIKELY(new_curr > 0)) {
		extern void gu_in_bytes_(GuIn* in, uint8_t* buf, size_t sz, 
					 GuError* err);
		gu_in_bytes_(in, buf, sz, err);
		return;
	}
	memcpy(buf, &in->buf_end[curr], sz);
	in->buf_curr = new_curr;
}

inline uint8_t 
gu_in_u8(GuIn* restrict in, GuError* err)
{
	if (GU_UNLIKELY(in->buf_curr == 0)) {
		extern uint8_t gu_in_u8_(GuIn* restrict in, GuError* err);
		return gu_in_u8_(in, err);
	}
	return in->buf_end[in->buf_curr++];
}

int8_t 
gu_in_s8(GuIn* in, GuError* err);

uint16_t
gu_in_u16le(GuIn* in, GuError* err);

uint16_t
gu_in_u16be(GuIn* in, GuError* err);

int16_t
gu_in_s16le(GuIn* in, GuError* err);

int16_t
gu_in_s16be(GuIn* in, GuError* err);

uint32_t
gu_in_u32le(GuIn* in, GuError* err);

uint32_t
gu_in_u32be(GuIn* in, GuError* err);

int32_t
gu_in_s32le(GuIn* in, GuError* err);

int32_t
gu_in_s32be(GuIn* in, GuError* err);

uint64_t
gu_in_u64le(GuIn* in, GuError* err);

uint64_t
gu_in_u64be(GuIn* in, GuError* err);

int64_t
gu_in_s64le(GuIn* in, GuError* err);

int64_t
gu_in_s64be(GuIn* in, GuError* err);

double
gu_in_f64le(GuIn* in, GuError* err);

double
gu_in_f64be(GuIn* in, GuError* err);

GuIn*
gu_buffered_in(GuIn* in, GuPool* pool);

GuIn*
gu_data_in(const uint8_t* buf, size_t size, GuPool* pool);


extern GU_DECLARE_TYPE(GuEOF, abstract);

#include <gu/type.h>

#endif // GU_IN_H_
