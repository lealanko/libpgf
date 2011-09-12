#ifndef GU_BITS_H_
#define GU_BITS_H_

#include <gu/defs.h>
#include <gu/assert.h>

typedef uintptr_t GuWord;

enum {
	GU_WORD_BITS = sizeof(GuWord) * CHAR_BIT
};


/*
 * Based on the Bit Twiddling Hacks collection by Sean Eron Anderson
 * <http://graphics.stanford.edu/~seander/bithacks.html>
 */

unsigned gu_ceil2e(unsigned i);

static inline int
gu_sign(int i) {
	return (i > 0) - (i < 0);
}

static inline size_t
gu_ceildiv(size_t size, size_t div)
{
	return (size + div - 1) / div;
}

static inline bool
gu_aligned(uintptr_t addr, size_t alignment)
{
	gu_require(alignment == gu_ceil2e(alignment));
	return (addr & (alignment - 1)) == 0;
}

static inline uintptr_t
gu_align_forward(uintptr_t addr, size_t alignment) {
	gu_require(alignment == gu_ceil2e(alignment));
	uintptr_t mask = alignment - 1;
	return (addr + mask) & ~mask;
}

static inline uintptr_t
gu_align_backward(uintptr_t addr, size_t alignment) {
	gu_require(alignment == gu_ceil2e(alignment));
	return addr & ~(alignment - 1);
}

static inline bool
gu_bits_test(const GuWord* bitmap, int idx) {
	return !!(bitmap[idx / GU_WORD_BITS] & 1 << (idx % GU_WORD_BITS));
}

static inline void
gu_bits_set(GuWord* bitmap, int idx) {
	bitmap[idx / GU_WORD_BITS] |= ((GuWord) 1) << (idx % GU_WORD_BITS);
}

static inline void
gu_bits_clear(GuWord* bitmap, int idx) {
	bitmap[idx / GU_WORD_BITS] &= ~(((GuWord) 1) << (idx % GU_WORD_BITS));
}

static inline size_t
gu_bits_size(size_t n_bits) {
	return gu_ceildiv(n_bits, GU_WORD_BITS) * sizeof(GuWord);
}


static inline int
gu_word_tag(GuWord w, size_t alignment) {
	return (int) (w & (alignment - 1));
}

static inline void*
gu_word_ptr(GuWord w, size_t alignment) {
	return (void*) gu_align_backward(w, alignment);
}

static inline GuWord
gu_word(void* ptr, size_t alignment, unsigned tag) {
	gu_require(tag < alignment);
	uintptr_t u = (uintptr_t) ptr;
	gu_require(gu_align_backward(u, alignment) == u);
	return (GuWord) { u | tag };
}

#define GuTagged() struct { GuWord w_; }

typedef GuTagged() GuTagged_;

#include <gu/error.h>
#include <gu/type.h>

typedef struct { int dummy_; } GuIntDecodeError;

GU_DECLARE_TYPE(GuIntDecodeError, abstract);

#define GU_DECODE_2C_(u_, t_, umax_, posmax_, tmin_, err_)	\
	(((u_) <= (posmax_))					\
	 ? (t_) (u_)						\
	 : (tmin_) + ((t_) ((umax_) - (u_))) < 0		\
	 ? -1 - ((t_) ((umax_) - (u_)))				\
	 : (gu_raise(err_, GuIntDecodeError, 0), -1))


static inline int8_t
gu_decode_2c8(uint8_t u, GuError* err)
{
	return GU_DECODE_2C_(u, int8_t, UINT8_C(0xff), 
			     INT8_C(0x7f), INT8_MIN, err);
}

static inline int16_t
gu_decode_2c16(uint16_t u, GuError* err)
{
	return GU_DECODE_2C_(u, int16_t, UINT16_C(0xffff), 
			     INT16_C(0x7fff), INT16_MIN, err);
}

static inline int32_t
gu_decode_2c32(uint32_t u, GuError* err)
{
	return GU_DECODE_2C_(u, int32_t, UINT32_C(0xffffffff), 
			     INT32_C(0x7fffffff), INT32_MIN, err);
}

static inline int64_t
gu_decode_2c64(uint64_t u, GuError* err)
{
	return GU_DECODE_2C_(u, int64_t, UINT64_C(0xffffffffffffffff), 
			     INT64_C(0x7fffffffffffffff), INT64_MIN, err);
}

double
gu_decode_double(uint64_t u);



#endif // GU_BITS_H_
