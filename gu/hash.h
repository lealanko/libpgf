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

struct GuHasher {
	GuEquality eq;
	GuHash (*hash)(GuHasher* self, GuHash h, const void* p);
};

static inline GuHash
gu_hasher_hash(GuHasher* hasher, const void* p)
{
	return hasher->hash(hasher, 0, p);
}


extern GuHasher gu_int_hasher[1];

extern GuHasher gu_addr_hasher[1];

extern GuHasher gu_word_hasher[1];


#include <gu/type.h>

extern GuTypeTable gu_hasher_instances;


#endif // GU_HASH_H_
