#include <gu/hash.h>
#include <gu/seq.h>
#include <gu/variant.h>
#include <gu/instance.h>

GuHash
gu_hash_bytes(GuHash h, const uint8_t* buf, size_t len)
{
	for (size_t n = 0; n < len; n++) {
		h = gu_hash_byte(h, buf[n]);
	}
	return h;
}

static GuHash
gu_hash_word(GuHash h, GuWord w)
{
	return gu_hash_bytes(h, (void*)&w, sizeof(GuWord));
}

static bool
gu_int_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	(void) self;
	const int* ip1 = p1;
	const int* ip2 = p2;
	return *ip1 == *ip2;
}

static GuHash
gu_int_hash_fn(GuHasher* self, GuHash h, const void* p)
{
	(void) self;
	return gu_hash_word(h, (GuWord) *(const int*) p);
}

GuHasher gu_int_hasher[1] = {
	{
		{ gu_int_eq_fn },
		gu_int_hash_fn
	}
};

static bool
gu_addr_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	(void) self;
	return (p1 == p2);
}

static GuHash
gu_addr_hash_fn(GuHasher* self, GuHash h, const void* p)
{
	(void) self;
	return gu_hash_word(h, (GuWord) p);
}

GuHasher gu_addr_hasher[1] = {
	{
		{ gu_addr_eq_fn },
		gu_addr_hash_fn
	}
};

static bool
gu_word_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	(void) self;
	const GuWord* wp1 = p1;
	const GuWord* wp2 = p2;
	return (*wp1 == *wp2);
}

static GuHash
gu_word_hash_fn(GuHasher* self, GuWord h, const void* p)
{
	(void) self;
	return gu_hash_bytes(h, p, sizeof(GuWord));
}

GuHasher gu_word_hasher[1] = {
	{
		{ gu_word_eq_fn },
		gu_word_hash_fn
	}
};



typedef struct GuSeqHasher GuSeqHasher;

struct GuSeqHasher {
	GuHasher hasher;
	GuHasher* elem_hasher;
	size_t elem_size;
};

static bool
gu_seq_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	GuSeqHasher* shasher = (GuSeqHasher*) self;
	GuEquality* eeq = &shasher->elem_hasher->eq;
	GuSeq s1 = *(const GuSeq*) p1;
	GuSeq s2 = *(const GuSeq*) p2;
	size_t len = gu_seq_length(s1);
	if (gu_seq_length(s2) != len) {
		return false;
	}
	const uint8_t* d1 = gu_seq_data(s1);
	const uint8_t* d2 = gu_seq_data(s2);
	for (size_t i = 0; i < len; i++) {
		const void* ep1 = &d1[i * shasher->elem_size];
		const void* ep2 = &d2[i * shasher->elem_size];
		if (!eeq->is_equal(eeq, ep1, ep2)) {
			return false;
		}
	}

	return true;
}

static GuHash
gu_seq_hash_fn(GuHasher* self, GuWord h, const void* p)
{
	GuSeqHasher* shasher = (GuSeqHasher*) self;
	GuHasher* ehasher = shasher->elem_hasher;
	GuSeq seq = *(const GuSeq*) p;
	size_t len = gu_seq_length(seq);
	h = gu_hash_word(h, len);
	uint8_t* data = gu_seq_data(seq);
	for (size_t i = 0; i < len; i++) {
		h = ehasher->hash(ehasher, h, &data[i * shasher->elem_size]);
	}
	return h;
}

static const void*
gu_make_seq_hasher(GuInstance* self, GuInstanceMap* imap,
		   GuType* type, GuPool* pool)
{
	(void) self;
	GuSeqType* stype = gu_type_cast(type, GuSeq);
	GuHasher* ehasher = gu_instantiate(imap, stype->elem_type, pool);
	size_t esize = gu_type_size(stype->elem_type);
	return gu_new_i(pool, GuSeqHasher,
			.hasher.eq.is_equal = gu_seq_eq_fn,
			.hasher.hash = gu_seq_hash_fn,
			.elem_hasher = ehasher,
			.elem_size = esize);
}


typedef struct GuVariantHasher GuVariantHasher;

struct GuVariantHasher {
	GuHasher hasher;
	GuSeq data_hashers;
};


static bool
gu_variant_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	GuVariantHasher* vhasher = (GuVariantHasher*) self;
	GuVariantInfo i1 = gu_variant_open(*(const GuVariant*) p1);
	GuVariantInfo i2 = gu_variant_open(*(const GuVariant*) p2);
	if (i1.tag != i2.tag) {
		return false;
	}
	GuHasher* dhasher =
		gu_seq_get(vhasher->data_hashers, GuHasher*, i1.tag);
	GuEquality* deq = &dhasher->eq;
	return deq->is_equal(deq, i1.data, i2.data);
}

static GuHash
gu_variant_hash_fn(GuHasher* self, GuWord h, const void* p)
{
	GuVariantHasher* vhasher = (GuVariantHasher*) self;
	GuVariantInfo i = gu_variant_open(*(const GuVariant*) p);
	h = gu_hash_word(h, i.tag);
	GuHasher* dhasher = gu_seq_get(vhasher->data_hashers, GuHasher*, i.tag);
	return dhasher->hash(dhasher, h, i.data);
}

static const void*
gu_make_variant_hasher(GuInstance* self, GuInstanceMap* imap,
		       GuType* type, GuPool* pool)
{
	(void) self;
	GuVariantType* vtype = gu_type_cast(type, GuVariant);
	size_t len = vtype->ctors.len;
	GuSeq dhashers = gu_new_seq(GuHasher*, len, pool);
	GuHasher** dha = gu_seq_data(dhashers);
	for (size_t i = 0; i < len; i++) {
		dha[i] = gu_instantiate(imap, vtype->ctors.elems[i].type,
					pool);
	}
	return gu_new_i(pool, GuVariantHasher,
			.hasher.eq.is_equal = gu_variant_eq_fn,
			.hasher.hash = gu_variant_hash_fn,
			.data_hashers = dhashers);
}

typedef struct GuStructHasher GuStructHasher;

typedef struct GuHasherMember GuHasherMember;

struct GuHasherMember {
	ptrdiff_t offset;
	GuHasher* hasher;
};

struct GuStructHasher {
	GuHasher hasher;
	GuSeq members;
};

static bool
gu_struct_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	GuStructHasher* shasher = (GuStructHasher*) self;
	const uint8_t* u1 = p1;
	const uint8_t* u2 = p2;
	size_t n_members = gu_seq_length(shasher->members);
	GuHasherMember* members = gu_seq_data(shasher->members);
	for (size_t i = 0; i < n_members; i++) {
		GuHasherMember* m = &members[i];
		const void* m1 = &u1[m->offset];
		const void* m2 = &u2[m->offset];
		GuEquality* eq = &m->hasher->eq;
		bool is_eq = eq->is_equal(eq, m1, m2);
		if (!is_eq) {
			return false;
		}
	}
	return true;
}

static GuHash
gu_struct_hash_fn(GuHasher* self, GuHash h, const void* p)
{
	GuStructHasher* shasher = (GuStructHasher*) self;
	const uint8_t* u = p;
	size_t n_members = gu_seq_length(shasher->members);
	GuHasherMember* members = gu_seq_data(shasher->members);
	for (size_t i = 0; i < n_members; i++) {
		GuHasherMember m = members[i];
		h = m.hasher->hash(m.hasher, h, &u[m.offset]);
	}
	return h;
}

static const void*
gu_make_struct_hasher(GuInstance* self, GuInstanceMap* imap,
		      GuType* type, GuPool* pool)
{
	(void) self;
	GuStructRepr* srepr = gu_type_cast(type, struct);
	size_t n_members = srepr->members.len;
	GuMember* members = srepr->members.elems;
	GuSeq hmembers = gu_new_seq(GuHasherMember, n_members, pool);
	GuHasherMember* hms = gu_seq_data(hmembers);
	for (size_t i = 0; i < n_members; i++) {
		hms[i].offset = members[i].offset;
		hms[i].hasher = gu_instantiate(imap, members[i].type, pool);
	}
	return gu_new_i(pool, GuStructHasher,
			.hasher.eq.is_equal = gu_struct_eq_fn,
			.hasher.hash = gu_struct_hash_fn,
			.members = hmembers);
}

typedef struct GuPointerHasher GuPointerHasher;

struct GuPointerHasher {
	GuHasher hasher;
	GuHasher* pointed_hasher;
};

static bool
gu_ptr_eq_fn(GuEquality* self, const void* p1, const void* p2)
{
	GuPointerHasher* phasher = (GuPointerHasher*) self;
	const void* v1 = *(const void* const *) p1;
	const void* v2 = *(const void* const *) p2;
	GuEquality* veq = &phasher->pointed_hasher->eq;
	return veq->is_equal(veq, v1, v2);
}

static GuHash
gu_ptr_hash_fn(GuHasher* self, GuHash h, const void* p)
{
	GuPointerHasher* phasher = (GuPointerHasher*) self;
	const void* v = *(const void* const *) p;
	GuHasher* vhasher = phasher->pointed_hasher;
	return vhasher->hash(vhasher, h, v);
}

static const void*
gu_make_ptr_hasher(GuInstance* self, GuInstanceMap* imap,
		   GuType* type, GuPool* pool)
{
	GuPointerType* ptype = gu_type_cast(type, pointer);
	GuHasher* pointed_hasher =
		gu_instantiate(imap, ptype->pointed_type, pool);
	return gu_new_i(pool, GuPointerHasher,
			.hasher.eq.is_equal = gu_ptr_eq_fn,
			.hasher.hash = gu_ptr_hash_fn,
			.pointed_hasher = pointed_hasher);
}


GuTypeTable gu_hasher_instances = GU_TYPETABLE(
	GU_SLIST_0,
	{ gu_kind(int), GU_CONST_INSTANCE(gu_int_hasher) },
	{ gu_kind(GuWord), GU_CONST_INSTANCE(gu_word_hasher) },
	{ gu_kind(struct), GU_INSTANCE(gu_make_struct_hasher) },
	{ gu_kind(GuVariant), GU_INSTANCE(gu_make_variant_hasher) },
	{ gu_kind(pointer), GU_INSTANCE(gu_make_ptr_hasher) },
	{ gu_kind(shared), GU_CONST_INSTANCE(gu_addr_hasher) },
	{ gu_kind(GuSeq), GU_INSTANCE(gu_make_seq_hasher) },
	);
	
