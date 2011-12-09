#include <gu/in.h>
#include <gu/bits.h>
#include <math.h>

GU_DEFINE_TYPE(GuEOF, abstract, _);

GuIn*
gu_make_in(GuInStream* stream, GuPool* pool)
{
	GuIn* in = gu_new(pool, GuIn);
	*in = gu_init_in(stream);
	return in;
}

static bool
gu_in_refill(GuIn* in, GuError* err)
{
	gu_assert(in->buf_curr == 0);
	GuInStream* stream = in->stream;
	if (!stream->next_buffer) {
		return false;
	}
	size_t sz = 0;
	const uint8_t* new_buf = stream->next_buffer(stream, &sz, err);
	if (new_buf) {
		in->buf_end = &new_buf[sz];
		in->buf_curr = -(ptrdiff_t) sz;
		return true;
	}
	return false;
}

static size_t
gu_in_input(GuIn* in, uint8_t* dst, size_t sz, GuError* err)
{
	if (sz == 0) {
		return 0;
	}
	GuInStream* stream = in->stream;
	if (stream->input) {
		return stream->input(stream, dst, sz, err);
	}
	gu_raise(err, GuEOF);
	return 0;
}

size_t
gu_in_some(GuIn* in, uint8_t* dst, size_t sz, GuError* err)
{
	gu_require(sz <= PTRDIFF_MAX);
	if (in->buf_curr == 0) {
		gu_in_refill(in, err);
		if (!gu_ok(err)) {
			return 0;
		}
	}
	if (in->buf_curr == 0) {
		return gu_in_input(in, dst, sz, err);
	}
	size_t real_sz = GU_MIN(sz, (size_t)(-in->buf_curr));
	memcpy(dst, &in->buf_end[in->buf_curr], real_sz);
	in->buf_curr += real_sz;
	return real_sz;
}

void
gu_in_bytes_(GuIn* in, uint8_t* dst, size_t sz, GuError* err)
{
	size_t have = 0;
	while (have < sz && gu_ok(err)) {
		size_t got = gu_in_some(in, &dst[have], sz - have, err);
		if (got == 0) {
			gu_raise(err, GuEOF);
			return;
		}
		have += got;
	}
}

uint8_t 
gu_in_u8_(GuIn* in, GuError* err)
{
	if (gu_in_refill(in, err) && in->buf_curr < 0) {
		return in->buf_end[in->buf_curr++];
	}
	uint8_t u = 0;
	size_t r = gu_in_input(in, &u, 1, err);
	if (r < 1) {
		gu_raise(err, GuEOF);
		return 0;
	}
	return u;
}

static uint64_t
gu_in_be(GuIn* in, GuError* err, int n)
{
	uint8_t buf[8];
	gu_in_bytes(in, buf, n, err);
	uint64_t u = 0;
	for (int i = 0; i < n; i++) {
		u = u << 8 | buf[i];
	}
	return u;
}

static uint64_t
gu_in_le(GuIn* in, GuError* err, int n)
{
	uint8_t buf[8];
	gu_in_bytes(in, buf, n, err);
	uint64_t u = 0;
	for (int i = 0; i < n; i++) {
		u = u << 8 | buf[i];
	}
	return u;
}

int8_t 
gu_in_s8(GuIn* in, GuError* err)
{
	return gu_decode_2c8(gu_in_u8(in, err), err);
}


uint16_t
gu_in_u16le(GuIn* in, GuError* err)
{
	return gu_in_le(in, err, 2);
}

int16_t 
gu_in_s16le(GuIn* in, GuError* err)
{
	return gu_decode_2c16(gu_in_u16le(in, err), err);
}

uint16_t
gu_in_u16be(GuIn* in, GuError* err)
{
	return gu_in_be(in, err, 2);
}

int16_t 
gu_in_s16be(GuIn* in, GuError* err)
{
	return gu_decode_2c16(gu_in_u16be(in, err), err);
}


uint32_t
gu_in_u32le(GuIn* in, GuError* err)
{
	return gu_in_le(in, err, 4);
}

int32_t 
gu_in_s32le(GuIn* in, GuError* err)
{
	return gu_decode_2c32(gu_in_u32le(in, err), err);
}

uint32_t
gu_in_u32be(GuIn* in, GuError* err)
{
	return gu_in_be(in, err, 4);
}

int32_t 
gu_in_s32be(GuIn* in, GuError* err)
{
	return gu_decode_2c32(gu_in_u32be(in, err), err);
}


uint64_t
gu_in_u64le(GuIn* in, GuError* err)
{
	return gu_in_le(in, err, 8);
}

int64_t 
gu_in_s64le(GuIn* in, GuError* err)
{
	return gu_decode_2c64(gu_in_u64le(in, err), err);
}

uint64_t
gu_in_u64be(GuIn* in, GuError* err)
{
	return gu_in_be(in, err, 8);
}

int64_t 
gu_in_s64be(GuIn* in, GuError* err)
{
	return gu_decode_2c64(gu_in_u64be(in, err), err);
}

double
gu_in_f64le(GuIn* in, GuError* err)
{
	return gu_decode_double(gu_in_u64le(in, err));
}

double
gu_in_f64be(GuIn* in, GuError* err)
{
	return gu_decode_double(gu_in_u64le(in, err));
}

enum {
	GU_BUFFERED_IN_BUF_SIZE = 4096
};

typedef struct GuBufferedInStream GuBufferedInStream;

struct GuBufferedInStream {
	GuInStream stream;
	uint8_t* buf;
	size_t sz;
	GuIn* in;
};

static const uint8_t*
gu_buffered_in_next(GuInStream* self, size_t* sz_out, GuError* err)
{
	GuBufferedInStream* bis = 
		gu_container(self, GuBufferedInStream, stream);
	*sz_out = gu_in_some(bis->in, bis->buf, bis->sz, err);
	if (!gu_ok(err)) {
		return NULL;
	}
	return bis->buf;
}

static size_t
gu_buffered_in_input(GuInStream* self, uint8_t* dst, size_t sz, GuError* err)
{
	GuBufferedInStream* bis = 
		gu_container(self, GuBufferedInStream, stream);
	return gu_in_some(bis->in, dst, sz, err);
}

GuIn*
gu_buffered_in(GuIn* in, GuPool* pool)
{
	uint8_t* buf = gu_malloc(pool, GU_BUFFERED_IN_BUF_SIZE);
	GuBufferedInStream* bis = 
		gu_new_s(pool, GuBufferedInStream,
			 .stream.next_buffer = gu_buffered_in_next,
			 .stream.input = gu_buffered_in_input,
			 .buf = buf,
			 .sz = GU_BUFFERED_IN_BUF_SIZE);
	return gu_make_in(&bis->stream, pool);
}

typedef struct GuDataIn GuDataIn;

struct GuDataIn {
	GuInStream stream; 
	const uint8_t* data;
	size_t sz;
};

static const uint8_t*
gu_data_in_next(GuInStream* self, size_t* sz_out, GuError* err)
{
	(void) err;
	GuDataIn* di = gu_container(self, GuDataIn, stream);
	const uint8_t* buf = di->data;
	if (buf) {
		*sz_out = di->sz;
		di->data = NULL;
		di->sz = 0;
	}
	return buf;
}

GuIn*
gu_data_in(const uint8_t* data, size_t sz, GuPool* pool)
{
	GuDataIn* di = gu_new_s(pool, GuDataIn,
			       .stream.next_buffer = gu_data_in_next,
			       .data = data,
			       .sz = sz);
	return gu_make_in(&di->stream, pool);
}

extern inline GuIn
gu_init_in(GuInStream* stream);

extern inline uint8_t 
gu_in_u8(GuIn* restrict in, GuError* err);

extern inline void
gu_in_bytes(GuIn* in, uint8_t* buf, size_t sz, GuError* err);

