// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef GU_HASH_H_
#define GU_HASH_H_

#include <gu/fun.h>

typedef GuWord GuHash;

static inline GuHash
gu_hash_ptr(void* ptr)
{
	return (GuHash) ptr;
}


static inline GuHash
gu_hash_byte(GuHash h, uint8_t u)
{
	// Paul Larson's simple byte hash
	return h * 101 + u;
}

GuHash
gu_hash_bytes(GuHash h, const uint8_t* buf, size_t len);



typedef const struct GuHasher GuHasher;

typedef const struct GuHasherFuns GuHasherFuns;

struct GuHasherFuns {
	GuHash (*hash)(GuHasher* self, GuHash h, const void* p);
};

struct GuHasher {
	GuHasherFuns* funs;
	GuEq* eq;
};

static inline GuHash
gu_hasher_hash(const GuHasher* hasher, const void* p)
{
	return gu_invoke(hasher, hash, 0, p);
}

#define GU_DEFINE_HASHER(NAME, HASH_FN, EQ_FN)				\
	GuHasher NAME[1] = {						\
		{							\
			.funs = &(GuHasherFuns) {			\
				.hash = HASH_FN				\
			},						\
			.eq = &(GuEq) {					\
				 .funs = &(GuEqFuns) {			\
					.is_equal = EQ_FN		\
				}					\
			}						\
		}							\
	}

extern GuHasher gu_int32_hasher[1];

extern GuHasher gu_addr_hasher[1];

extern GuHasher gu_word_hasher[1];


#include <gu/type.h>

extern GuTypeTable gu_hasher_instances[1];


#endif // GU_HASH_H_
