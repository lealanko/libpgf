#include <gu/in.h>
#include <gu/bits.h>
#include <math.h>

GU_DEFINE_TYPE(GuEOF, abstract, _);

size_t
gu_in_some(GuIn* in, uint8_t* buf, size_t len, GuError* err)
{
	return in->input(in, buf, len, err);
}

void
gu_in_bytes(GuIn* in, uint8_t* buf, size_t len, GuError* err)
{
	size_t have = 0;
	while (have < len && gu_ok(err)) {
		size_t got = gu_in_some(in, &buf[have], len - have, err);
		if (got == 0) {
			gu_raise(err, GuEOF);
			return;
		}
		have += got;
	}
}

uint8_t 
gu_in_u8(GuIn* in, GuError* err)
{
	uint8_t ret = 0xfe;
	gu_in_bytes(in, &ret, 1, err);
	return ret;
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


typedef struct GuBufIn GuBufIn;

struct GuBufIn {
	GuIn in;
	const uint8_t* buf;
	size_t len;
};

static size_t
gu_buf_input(GuIn* in, uint8_t* buf, size_t max_len, GuError* err)
{
	(void) err;
	GuBufIn* bin = (GuBufIn*) in;
	size_t len = GU_MIN(max_len, bin->len);
	memcpy(buf, bin->buf, len);
	bin->buf += len;
	bin->len -= len;
	return len;
}

GuIn*
gu_buf_in(const uint8_t* buf, size_t len, GuPool* pool)
{
	return (GuIn*) gu_new_s(pool, GuBufIn,
				.in = { gu_buf_input },
				.buf = buf,
				.len = len);
}

