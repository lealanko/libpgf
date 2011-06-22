#ifndef GU_BITS_H_
#define GU_BITS_H_

#include <gu/defs.h>
#include <gu/assert.h>

typedef unsigned long GuWord;

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

static inline uintptr_t
gu_align_forward(uintptr_t addr, size_t alignment) {
	gu_assert(alignment == gu_ceil2e(alignment));
	uintptr_t mask = alignment - 1;
	return (addr + mask) & ~mask;
}

static inline uintptr_t
gu_align_backward(uintptr_t addr, size_t alignment) {
	gu_assert(alignment == gu_ceil2e(alignment));
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


#endif // GU_BITS_H_
