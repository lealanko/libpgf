#ifndef GU_OUT_H_
#define GU_OUT_H_

#include <gu/defs.h>
#include <gu/error.h>

typedef const struct GuOut GuOut;

struct GuOut {
	size_t (*output)(GuOut* self, const uint8_t* buf, size_t size, GuError* err);
};

size_t
gu_out_bytes(GuOut* out, const uint8_t* buf, size_t len, GuError* err);

void
gu_out_s8(GuOut* out, int8_t i, GuError* err);

void
gu_out_u8(GuOut* out, uint8_t u, GuError* err);

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

#include <gu/seq.h>

GuOut*
gu_byte_seq_out(GuByteSeq byteq, GuPool* pool);

#endif // GU_OUT_H_
