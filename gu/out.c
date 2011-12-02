#include <gu/seq.h>
#include <gu/out.h>


size_t
gu_out_bytes(GuOut* out, const uint8_t* buf, size_t len, GuError* err)
{
	if (!gu_ok(err)) {
		return 0;
	}
	return out->output(out, buf, len, err);
}

void
gu_out_flush(GuOut* out, GuError* err)
{
	if (out->flush) {
		out->flush(out, err);
	}
}


void
gu_out_s8(GuOut* out, int8_t i, GuError* err)
{
	uint8_t u = i >= 0 ? (uint8_t) i : (0x100 + i);
	gu_out_u8(out, u, err);
}


typedef struct GuBytesOut GuBytesOut;
struct GuBytesOut
{
	GuOut out;
	GuByteBuf* bbuf;
};

static size_t
gu_bytes_output(GuOut* out, const uint8_t* buf, size_t len, GuError* err)
{
	(void) err;
	GuBytesOut* bout = (GuBytesOut*) out;
	gu_buf_push_n(bout->bbuf, buf, len);
	return len;
}

GuOut*
gu_buf_out(GuByteBuf* bbuf, GuPool* pool)
{
	return (GuOut*) gu_new_s(pool, GuBytesOut,
				 .out = { .output = gu_bytes_output },
				 .bbuf = bbuf);
}
