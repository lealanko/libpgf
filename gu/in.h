#ifndef GU_IN_H_
#define GU_IN_H_

#include <gu/defs.h>
#include <gu/error.h>

typedef const struct GuIn GuIn;

struct GuIn {
	size_t (*input)(GuIn* self, uint8_t* buf, size_t max_len, GuError* err);
};

size_t
gu_in_some(GuIn* in, uint8_t* buf, size_t max_len, GuError* err);

void
gu_in_bytes(GuIn* in, uint8_t* buf, size_t len, GuError* err);

int8_t 
gu_in_s8(GuIn* in, GuError* err);

uint8_t 
gu_in_u8(GuIn* in, GuError* err);

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
gu_buf_in(const uint8_t* buf, size_t size, GuPool* pool);

extern GU_DECLARE_TYPE(GuEOF, abstract);

#include <gu/type.h>

#endif // GU_IN_H_
