#ifndef GU_BITS_H_
#define GU_BITS_H_

/*
 * Based on the Bit Twiddling Hacks collection by Sean Eron Anderson
 * <http://graphics.stanford.edu/~seander/bithacks.html>
 */

unsigned gu_ceil2e(unsigned i);

static inline int
gu_sign(int i) {
	return (i > 0) - (i < 0);
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




#endif // GU_BITS_H_
