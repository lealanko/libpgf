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
gu_out_u8(GuOut* out, uint8_t u, GuError* err)
{
	gu_out_bytes(out, &u, 1, err);
}


void
gu_out_s8(GuOut* out, int8_t i, GuError* err)
{
	uint8_t u = i >= 0 ? (uint8_t) i : (0x100 + i);
	gu_out_u8(out, u, err);
}


typedef struct GuByteSeqOut GuByteSeqOut;
struct GuByteSeqOut
{
	GuOut out;
	GuByteSeq byteq;
};

static size_t
gu_byte_seq_output(GuOut* out, const uint8_t* buf, size_t len, GuError* err)
{
	(void) err;
	GuByteSeqOut* bout = (GuByteSeqOut*) out;
	gu_byte_seq_push_n(bout->byteq, buf, len);
	return len;
}

GuOut*
gu_byte_seq_out(GuByteSeq byteq, GuPool* pool)
{
	return (GuOut*) gu_new_s(pool, GuByteSeqOut,
				 .out = { .output = gu_byte_seq_output },
				 .byteq = byteq);
}
