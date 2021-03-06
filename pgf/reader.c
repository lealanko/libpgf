// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

#include "data.h"
#include "expr.h"
#include <gu/defs.h>
#include <gu/map.h>
#include <gu/seq.h>
#include <gu/assert.h>
#include <gu/intern.h>
#include <gu/in.h>
#include <gu/bits.h>
#include <gu/exn.h>
#include <gu/utf8.h>

#define GU_LOG_ENABLE
#include <gu/log.h>

typedef struct PgfIdContext PgfIdContext;

//
// PgfReader
// 

typedef struct PgfReader PgfReader;

struct PgfReader {
	GuIn* in;
	GuExn* err;
	GuPool* opool;
	GuPool* pool;
	GuSymTable* symtab;
	PgfSequences curr_sequences;
	PgfCncFuns curr_cncfuns;
	GuMap* curr_ccats;
	GuMap* curr_lindefs;
	GuTypeMap* read_to_map;
	GuTypeMap* read_new_map;
	void* curr_key;
	GuMap* ctx;
	GuPool* curr_pool;
};

typedef struct PgfReadTagExn PgfReadTagExn;

struct PgfReadTagExn {
	GuType* type;
	int tag;
};

static GU_DEFINE_TYPE(PgfReadTagExn, abstract, _);

GU_DEFINE_TYPE(PgfReadExn, abstract, _);

static void
pgf_reader_tell(PgfReader* rdr)
{
	gu_debug("stream_pos: %zx", gu_in_tell(rdr->in));
}

static uint8_t
pgf_read_u8(PgfReader* rdr)
{
	uint8_t u = gu_in_u8(rdr->in, rdr->err);
	gu_debug("u8: %u", u);
	return u;
}

static uint32_t
pgf_read_uint(PgfReader* rdr)
{
	uint32_t u = 0;
	int shift = 0;
	uint8_t b = 0;
	do {
		b = pgf_read_u8(rdr);
		gu_return_on_exn(rdr->err, 0);
		u |= (b & ~0x80) << shift;
		shift += 7;
	} while (b & 0x80);
	gu_debug("uint: %u", u);
	return u;
}

static int32_t
pgf_read_int(PgfReader* rdr)
{
	uint32_t u = pgf_read_uint(rdr);
	return gu_decode_2c32(u, rdr->err);
}

static GuLength
pgf_read_len(PgfReader* rdr)
{
	int32_t len = pgf_read_int(rdr);
	// It's crucial that we return 0 on failure, so the
	// caller can proceed without checking for error
	// immediately.
	gu_return_on_exn(rdr->err, 0);
	if (len < 0) {
		gu_raise_i(rdr->err, PgfReadTagExn,
			   .type = gu_type(GuLength), .tag = len);
		return 0;
	}
	return (GuLength) len;
}

typedef const struct PgfReadToFn PgfReadToFn;

struct PgfReadToFn {
	void (*fn)(GuType* type, PgfReader* rdr, void* to);
};

static void 
pgf_read_to(PgfReader* rdr, GuType* type, void* to) {
	PgfReadToFn* fn = gu_type_map_get(rdr->read_to_map, type);
	fn->fn(type, rdr, to);
}

typedef const struct PgfReadNewFn PgfReadNewFn;
struct PgfReadNewFn {
	void* (*fn)(GuType* type, PgfReader* rdr, GuPool* pool);
};

static void*
pgf_read_new(PgfReader* rdr, GuType* type, GuPool* pool)
{
	PgfReadNewFn* fn = gu_type_map_get(rdr->read_new_map, type);
	return fn->fn(type, rdr, pool);
}

static void*
pgf_read_new_type(GuType* type, PgfReader* rdr, GuPool* pool)
{
	GuTypeRepr* repr = gu_type_repr(type);
	void* to = gu_malloc_aligned(pool, repr->size, repr->align);
	pgf_read_to(rdr, type, to);
	return to;
}

static void
pgf_read_to_struct(GuType* type, PgfReader* rdr, void* to)
{
	GuStructRepr* stype = gu_type_cast(type, struct);
	uint8_t* bto = to;
	gu_enter("-> struct %s", stype->name);

	for (int i = 0; i < stype->members.len; i++) {
		const GuMember* m = &stype->members.elems[i];
		gu_enter("-> %s.%s", stype->name, m->name);
		pgf_read_to(rdr, m->type, &bto[m->offset]);
		gu_return_on_exn(rdr->err,);
		gu_exit("<- %s.%s", stype->name, m->name);
	}
	gu_exit("<- struct %s", stype->name);
}


static void
pgf_read_to_pointer(GuType* type, PgfReader* rdr, void* to)
{
	GuPointerType* ptype = (GuPointerType*) type;
	GuType* pointed = ptype->pointed_type;
	gu_require(gu_type_has_kind(pointed, gu_kind(struct)) ||
		   gu_type_has_kind(pointed, gu_kind(abstract)));
	GuStruct** sto = to;
	*sto = pgf_read_new(rdr, pointed, rdr->opool);
}

static void
pgf_read_to_GuVariant(GuType* type, PgfReader* rdr, void* to)
{
	GuVariantType* vtype = (GuVariantType*) type;

	uint8_t btag = pgf_read_u8(rdr);
	gu_return_on_exn(rdr->err,);
	if (btag >= vtype->ctors.len) {
		gu_raise_i(rdr->err, PgfReadTagExn, 
			   .type = type, .tag = btag);
		return;
	}
	GuConstructor* ctor = &vtype->ctors.elems[btag];
	gu_enter("-> variant %s", ctor->c_name);
	GuTypeRepr* repr = gu_type_repr(ctor->type);
	void* value = gu_alloc_variant(btag, repr->size, repr->align, to,
				       rdr->opool);
	pgf_read_to(rdr, ctor->type, value);
	gu_exit("<- variant %s", ctor->c_name);
}

static void
pgf_read_to_enum(GuType* type, PgfReader* rdr, void* to)
{
	// For now, assume that enum values are encoded in a single octet
	GuEnumType* etype = (GuEnumType*) type;
	uint8_t tag = pgf_read_u8(rdr);
	gu_return_on_exn(rdr->err,);
	if (tag >= etype->constants.len) {
		gu_raise_i(rdr->err, PgfReadTagExn, 
			   .type = type, .tag = tag);
		return;
	}
	GuEnumConstant* econ = &etype->constants.elems[tag];
	size_t size = gu_type_size(type);
	if (size == sizeof(int8_t)) {
		*((int8_t*) to) = econ->value;
	} else if (size == sizeof(int16_t)) {
		*((int16_t*) to) = econ->value;
	} else if (size == sizeof(int32_t)) {
		*((int32_t*) to) = econ->value;
	} else if (size == sizeof(int64_t)) {
		*((int64_t*) to) = econ->value;
	} else {
		gu_impossible();
	}
}

static void
pgf_read_to_void(GuType* info, PgfReader* rdr, void* to)
{
}


static void
pgf_read_to_int32_t(GuType* type, PgfReader* rdr, void* to) 
{
	*(int*) to = pgf_read_int(rdr);
}

static void
pgf_read_to_uint16_t(GuType* type, PgfReader* rdr, void* to)
{
	*(uint16_t*) to = gu_in_u16be(rdr->in, rdr->err);
}

static void
pgf_read_to_GuLength(GuType* type, PgfReader* rdr, void* to)
{
	*(GuLength*) to = pgf_read_len(rdr);
}

static void
pgf_read_to_double(GuType* type, PgfReader* rdr, void* to)
{
	*(double*) to = gu_in_f64be(rdr->in, rdr->err);
}

static void
pgf_read_to_alias(GuType* type, PgfReader* rdr, void* to)
{
	GuTypeAlias* atype = gu_type_cast(type, alias);
	pgf_read_to(rdr, atype->type, to);
}

static void
pgf_read_to_referenced(GuType* type, PgfReader* rdr, void* to)
{
	GuTypeAlias* atype = gu_type_cast(type, alias);
	gu_map_put(rdr->ctx, type, void*, to);
	pgf_read_to(rdr, atype->type, to);
}
static void
pgf_read_into_map(GuMapType* mtype, PgfReader* rdr, GuMap* map, GuPool* pool)
{
	/* The parameter pool is the temporary pool used to store the
	   map. But the actual values need to be more persistent so we
	   store them in rdr->opool. */
	GuPool* tmp_pool = gu_new_pool();
	void* old_key = rdr->curr_key;
	void* key = NULL;
	void* value = NULL;
	GuLength len = pgf_read_len(rdr);
	gu_return_on_exn(rdr->err, );
	if (mtype->hasher) {
		key = gu_type_malloc(mtype->key_type, tmp_pool);
	}
	value = gu_type_malloc(mtype->value_type, tmp_pool);
	for (size_t i = 0; i < len; i++) {
		if (mtype->hasher) {
			pgf_read_to(rdr, mtype->key_type, key);
		} else {
			key = pgf_read_new(rdr, mtype->key_type, 
					   rdr->opool);
		}
		gu_return_on_exn(rdr->err, );
		rdr->curr_key = key;
		pgf_read_to(rdr, mtype->value_type, value);
		gu_return_on_exn(rdr->err, );
		void* valp = gu_map_insert(map, key);
		gu_type_memcpy(valp, value, mtype->value_type);       
	}
	rdr->curr_key = old_key;
	gu_pool_free(tmp_pool);
}

static void*
pgf_read_new_GuMap(GuType* type, PgfReader* rdr, GuPool* pool)
{
	GuMapType* mtype = (GuMapType*) type;
	GuMap* map = gu_map_type_make(mtype, pool);
	pgf_read_into_map(mtype, rdr, map, pool);
	gu_return_on_exn(rdr->err, NULL);
	return map;
}

static void
pgf_read_to_GuString(GuType* type, PgfReader* rdr, void* to)
{
	gu_enter("-> GuString");
	pgf_reader_tell(rdr);
	GuString* sp = to;
	
	GuPool* tmp_pool = gu_new_pool();
	GuStringBuf* sbuf = gu_string_buf(tmp_pool);
	GuWriter* wtr = gu_string_buf_writer(sbuf);

	GuLength len = pgf_read_len(rdr);

	for (size_t i = 0; i < len; i++) {
		GuUCS ucs = gu_in_utf8(rdr->in, rdr->err);
		gu_ucs_write(ucs, wtr, rdr->err);
	}
	GuString str = gu_string_buf_freeze(sbuf, tmp_pool);
	GuSymbol sym = gu_symtable_intern(rdr->symtab, str);
	gu_pool_free(tmp_pool);

	gu_exit("<- GuString");
	*sp = sym;
}

static void
pgf_read_to_PgfCId(GuType* type, PgfReader* rdr, void* to)
{
	gu_enter("-> PgfCId");
	pgf_reader_tell(rdr);
	
	PgfCId* sp = to;
	
	GuPool* tmp_pool = gu_new_pool();
	GuStringBuf* sbuf = gu_string_buf(tmp_pool);
	GuWriter* wtr = gu_string_buf_writer(sbuf);

	GuLength len = pgf_read_len(rdr);

	for (size_t i = 0; i < len; i++) {
		// CIds are in latin-1
		GuUCS ucs = gu_in_u8(rdr->in, rdr->err);
		gu_ucs_write(ucs, wtr, rdr->err);
	}
	GuString str = gu_string_buf_freeze(sbuf, tmp_pool);
	GuSymbol sym = gu_symtable_intern(rdr->symtab, str);
	gu_pool_free(tmp_pool);

	gu_exit("<- PgfCId");
	*sp = sym;
}

static PgfCCat*
pgf_reader_intern_ccat(PgfReader* rdr, PgfFId fid)
{
	PgfCCat** ccatp = gu_map_insert(rdr->curr_ccats, &fid);
	if (!*ccatp) {
		*ccatp = gu_new_i(rdr->opool, PgfCCat,
				  .cnccat = NULL,
				  .prods = gu_null_seq);
		pgf_ccat_set_fid(*ccatp, fid);
		gu_debug("interned ccat %d@%p (into map loc %p)",
			 fid, *ccatp, ccatp);
	}
	return *ccatp;
}

static void
pgf_read_to_PgfCCatId(GuType* type, PgfReader* rdr, void* to)
{
	PgfCCat** pto = to;
	PgfFId fid = pgf_read_int(rdr);
	gu_return_on_exn(rdr->err,);
	*pto = pgf_reader_intern_ccat(rdr, fid);
}

static void*
pgf_read_new_PgfCCat(GuType* type, PgfReader* rdr, GuPool* pool)
{
	PgfFId* fidp = rdr->curr_key;
	PgfCCat* ccat = pgf_reader_intern_ccat(rdr, *fidp);
	pgf_read_to(rdr, gu_type(PgfProductions), &ccat->prods);
	return ccat;
}

static void
pgf_read_to_GuSeq(GuType* type, PgfReader* rdr, void* to)
{
	gu_enter("->");
	void* old_key = rdr->curr_key;
	GuSeqType* stype = gu_type_cast(type, GuSeq);
	GuLength length = pgf_read_len(rdr);
	GuTypeRepr* repr = gu_type_repr(stype->elem_type);
	gu_return_on_exn(rdr->err, );
	GuSeq seq = gu_make_seq(repr->size, length, rdr->opool);
	uint8_t* data = gu_seq_data(seq);
	for (size_t i = 0; i < length; i++) {
		rdr->curr_key = &i;
		void* elem = &data[i * repr->size];
		pgf_read_to(rdr, stype->elem_type, elem);
		gu_return_on_exn(rdr->err, );
	}
	GuSeq* sto = to;
	*sto = seq;
	rdr->curr_key = old_key;
	gu_exit("<-");
}

static void
pgf_read_to_maybe_seq(GuType* type, PgfReader* rdr, void* to)
{
	GuSeq* sto = to;
	uint8_t tag = pgf_read_u8(rdr);
	gu_return_on_exn(rdr->err,);
	switch (tag) {
	case 0:
		*sto = gu_null_seq;
		break;
	case 1:
		pgf_read_to_GuSeq(type, rdr, to);
		break;
	default:
		gu_raise_i(rdr->err, PgfReadTagExn,
			   .type = type, .tag = tag);
		break;
	}
}


static void
pgf_read_to_idarray(GuType* type, PgfReader* rdr, void* to)
{
	pgf_read_to_GuSeq(type, rdr, to);
	gu_return_on_exn(rdr->err,);
	GuSeq seq = *(GuSeq*) to;
	if (type == gu_type(PgfSequences)) {
		rdr->curr_sequences = seq;
	} else if (type == gu_type(PgfCncFuns)) {
		rdr->curr_cncfuns = seq;
	} else {
		gu_impossible();
	}
}

static void
pgf_read_to_PgfContext(GuType* type, PgfReader* rdr, void* to)
{
	GuPointerType* ptype = gu_type_cast(type, pointer);
	GuStruct* ctx = gu_map_get(rdr->ctx, ptype->pointed_type, GuStruct*);
	*(GuStruct**) to = ctx;
}

static void
pgf_read_to_PgfKey(GuType* type, PgfReader* rdr, void* to)
{
	size_t sz = gu_type_size(type);
	memcpy(to, rdr->curr_key, sz);
}

static void
pgf_read_to_PgfSeqId(GuType* type, PgfReader* rdr, void* to)
{
	int32_t id = pgf_read_int(rdr);
	gu_return_on_exn(rdr->err,);
	if (id < 0 || (size_t)id >= gu_seq_length(rdr->curr_sequences)) {
		gu_raise(rdr->err, PgfReadExn);
		return;
	}
	*(PgfSeqId*) to = gu_seq_get(rdr->curr_sequences, PgfSeqId, id);
}


static void
pgf_read_to_PgfFunId(GuType* type, PgfReader* rdr, void* to)
{
	int32_t id = pgf_read_int(rdr);
	gu_return_on_exn(rdr->err,);
	if (id < 0 || (size_t)id >= gu_seq_length(rdr->curr_cncfuns)) {
		gu_raise(rdr->err, PgfReadExn);
		return;
	}
	*(PgfFunId*) to = gu_seq_get(rdr->curr_cncfuns, PgfFunId, id);
}

static GU_DEFINE_TYPE(PgfLinDefs, GuMap,
		      gu_type(int32_t), gu_int32_hasher,
		      gu_type(PgfFunIds), &gu_null_struct);
typedef PgfCCat PgfCCatData;
static GU_DEFINE_TYPE(PgfCCatData, typedef, gu_type(PgfCCat));

static GU_DEFINE_TYPE(PgfCCatMap, GuMap,
		      gu_type(int32_t), gu_int32_hasher,
		      gu_ptr_type(PgfCCat), &gu_null_struct);

typedef struct {
	GuMapItor fn;
	GuBuf* seq;
} PgfCCatCbCtx;


// XXX TODO: turn this to a parser-specific index CCat* |-> CncCat*
static PgfCncCat*
pgf_ccat_set_cnccat(PgfCCat* ccat, GuBuf* newly_set)
{
	if (!ccat->cnccat) {
		size_t n_prods = gu_seq_length(ccat->prods);
		for (size_t i = 0; i < n_prods; i++) {
			PgfProduction prod = 
				gu_seq_get(ccat->prods, PgfProduction, i);
			GuVariantInfo i = gu_variant_open(prod);
			switch (i.tag) {
			case PGF_PRODUCTION_COERCE: {
				PgfProductionCoerce* pcoerce = i.data;
				PgfCncCat* cnccat = 
					pgf_ccat_set_cnccat(pcoerce->coerce,
							    newly_set);
				if (!ccat->cnccat) {
					ccat->cnccat = cnccat;
				} else if (ccat->cnccat != cnccat) {
					// XXX: real error
					gu_impossible();
				}
 				break;
			}
			case PGF_PRODUCTION_APPLY:
				// Shouldn't happen with current PGF.
				// XXX: real error
				gu_impossible();
				break;
			default:
				gu_impossible();
			}
		}
		gu_buf_push(newly_set, PgfCCat*, ccat);
	}
	return ccat->cnccat;
}

static void
pgf_read_ccat_cb(GuMapItor* fn, const void* key, void* value, GuExn* err)
{
	PgfCCatCbCtx* ctx = (PgfCCatCbCtx*) fn;
	PgfCCat** ccatp = value;
	pgf_ccat_set_cnccat(*ccatp, ctx->seq);
}

static void*
pgf_read_new_PgfConcr(GuType* type, PgfReader* rdr, GuPool* pool)
{
	/* We allocate indices from a temporary pool. The actual data
	 * is allocated from rdr->opool. Once everything is resolved
	 * and indices aren't needed, the temporary pool can be
	 * freed. */
	GuPool* tmp_pool = gu_new_pool();
	rdr->curr_pool = tmp_pool;
	PgfConcr* concr = gu_new(PgfConcr, pool);
	concr->pgf =
		(PgfPGF*) gu_map_get(rdr->ctx, gu_type(PgfPGF), GuStruct*);
	concr->id = *(PgfCId*) rdr->curr_key;
	
	concr->cflags = 
		pgf_read_new(rdr, gu_type(PgfFlags), pool);
	concr->printnames = 
		pgf_read_new(rdr, gu_type(PgfPrintNames), pool);
	pgf_read_to(rdr, gu_type(PgfSequences), &rdr->curr_sequences);
	pgf_read_to(rdr, gu_type(PgfCncFuns), &rdr->curr_cncfuns);
	if (!gu_ok(rdr->err)) {
		goto fail;
	}
#ifndef GU_OPTIMIZE_SIZE	
	PgfCncFun** cncfuns = gu_seq_data(rdr->curr_cncfuns);
	size_t n_cncfuns = gu_seq_length(rdr->curr_cncfuns);
	for (size_t i = 0; i < n_cncfuns; i++) {
		cncfuns[i]->fid = (PgfFId) i;
	}
#endif
	GuMapType* lindefs_t = gu_type_cast(gu_type(PgfLinDefs), GuMap);
	rdr->curr_lindefs = gu_map_type_make(lindefs_t, tmp_pool);
	pgf_read_into_map(lindefs_t, rdr, rdr->curr_lindefs, rdr->opool);
	GuMapType* ccats_t = gu_type_cast(gu_type(PgfCCatMap), GuMap);
	rdr->curr_ccats = gu_new_map(PgfFId, gu_int32_hasher,
				     PgfCCat*, &gu_null_struct, tmp_pool);
	pgf_read_into_map(ccats_t, rdr, rdr->curr_ccats, rdr->opool);
	concr->cnccats =
		pgf_read_new(rdr, gu_type(PgfCncCatMap), rdr->opool);

	GuBuf* extra_ccats = gu_new_buf(PgfCCat*, tmp_pool);
	PgfCCatCbCtx ctx = { { pgf_read_ccat_cb }, extra_ccats };
	gu_map_iter(rdr->curr_ccats, &ctx.fn, gu_null_exn());
	concr->extra_ccats = gu_buf_freeze(extra_ccats, rdr->opool);
	(void) pgf_read_int(rdr); // totalcats
fail:
	gu_pool_free(tmp_pool);
	return concr;
}

static bool
pgf_ccat_n_ctnts(PgfCCat* cat, int* n_ctnts) {
	if (gu_seq_is_null(cat->prods)) {
		return true;
	}
	size_t n_prods = gu_seq_length(cat->prods);
	for (size_t j = 0; j < n_prods; j++) {
		PgfProduction prod =
			gu_seq_get(cat->prods, PgfProduction, j);
		GuVariantInfo i = gu_variant_open(prod);
		switch (i.tag) {
		case PGF_PRODUCTION_APPLY: {
			PgfProductionApply* papp = i.data;
			int old_n_ctnts = (int) gu_seq_length(papp->fun->lins);
			if (*n_ctnts == -1) {
				*n_ctnts = old_n_ctnts;
			} else if (*n_ctnts != old_n_ctnts) {
				// Inconsistent n_ctnts for different productions!
				return false;
			}
			break;
		}
		case PGF_PRODUCTION_COERCE: {
			PgfProductionCoerce* pcoerce = i.data;
			bool succ = pgf_ccat_n_ctnts(pcoerce->coerce, n_ctnts);
			if (!succ) {
				return false;
			}
			break;
		}
		default:
			gu_impossible();
		}
	}
	return true;
}

static void*
pgf_read_new_PgfCatId(GuType* type, PgfReader* rdr, GuPool* pool)
{
	PgfPGF* pgf = gu_map_get(rdr->ctx, gu_type(PgfPGF), PgfPGF*);
	PgfCId cid;
	pgf_read_to_PgfCId(gu_type(PgfCId), rdr, &cid);
	if (!gu_ok(rdr->err)) return NULL;
	PgfCat* cat = gu_map_get(pgf->abstract.cats, &cid, PgfCat*);
	if (!cat) {
		cat = gu_new(PgfCat, rdr->opool);
		cat->pgf = pgf;
		cat->cid = cid;
		cat->context = gu_empty_seq();
		cat->functions = gu_empty_seq();
		gu_map_put(pgf->abstract.cats, &cid, PgfCat*, cat);
	}
	return cat;
}

static void*
pgf_read_new_PgfCncCat(GuType* type, PgfReader* rdr, GuPool* pool)
{
	PgfCId cid = *(PgfCId*) rdr->curr_key;
	gu_enter("-> cid");
	PgfCncCat* cnccat = gu_new(PgfCncCat, pool);
	cnccat->cid = cid;
	int first = pgf_read_int(rdr);
	int last = pgf_read_int(rdr);
	int len = last + 1 - first;
	PgfCCatIds cats = gu_new_seq(PgfCCatId, len, pool);
	int n_ctnts = -1;
	for (int i = 0; i < len; i++) {
		int n = first + i;
		PgfCCat* ccat = gu_map_get(rdr->curr_ccats, &n, PgfCCat*);
		/* ccat can be NULL if the PGF is optimized and the
		 * category has been erased as useless */
		gu_seq_set(cats, PgfCCatId, i, ccat);
		if (ccat != NULL) {
			// TODO: error if overlap
			ccat->cnccat = cnccat;
			if (!pgf_ccat_n_ctnts(ccat, &n_ctnts)) {
				gu_raise(rdr->err, PgfReadExn);
				goto fail;
			}
			
		}
		gu_debug("range[%d] = %d", i, ccat ? pgf_ccat_fid(ccat) : -1);
	}
	cnccat->n_ctnts = n_ctnts == -1 ? 0 : (size_t) n_ctnts;
	cnccat->cats = cats;
	cnccat->lindefs = gu_map_get(rdr->curr_lindefs, &first, PgfFunIds);
	pgf_read_to(rdr, gu_type(GuStrings), &cnccat->ctnts);
	gu_exit("<-");
	return cnccat;
fail:
	gu_exit("<- fail");
	return NULL;
}

#define PGF_READ_TO_FN(k_, fn_)					\
	{ gu_kind(k_), (void*) &(PgfReadToFn){ fn_ } }

#define PGF_READ_TO(k_)				\
	PGF_READ_TO_FN(k_, pgf_read_to_##k_)


static GuTypeTable
pgf_read_to_table = GU_TYPETABLE(
	GU_SLIST_0,
	PGF_READ_TO(struct),
	PGF_READ_TO(GuVariant),
	PGF_READ_TO(enum),
	PGF_READ_TO(void),
	PGF_READ_TO(int32_t),
	PGF_READ_TO(uint16_t),
	PGF_READ_TO(GuLength),
	PGF_READ_TO(PgfCId),
	PGF_READ_TO(GuString),
	PGF_READ_TO(double),
	PGF_READ_TO(pointer),
	PGF_READ_TO_FN(PgfEquationsM, pgf_read_to_maybe_seq),
	PGF_READ_TO(GuSeq),
	PGF_READ_TO(PgfCCatId),
	PGF_READ_TO(PgfSeqId),
	PGF_READ_TO(PgfFunId),
	PGF_READ_TO(PgfContext),
	PGF_READ_TO(PgfKey),
	PGF_READ_TO(alias),
	PGF_READ_TO(referenced),
	PGF_READ_TO_FN(PgfSequences, pgf_read_to_idarray),
	PGF_READ_TO_FN(PgfCncFuns, pgf_read_to_idarray));

#define PGF_READ_NEW_FN(k_, fn_)		\
	{ gu_kind(k_), (void*) &(PgfReadNewFn){ fn_ } }

#define PGF_READ_NEW(k_)			\
	PGF_READ_NEW_FN(k_, pgf_read_new_##k_)

static GuTypeTable
pgf_read_new_table = GU_TYPETABLE(
	GU_SLIST_0,
	PGF_READ_NEW(type),
	PGF_READ_NEW(GuMap),
	PGF_READ_NEW(PgfCatId),
	PGF_READ_NEW(PgfCCat),
	PGF_READ_NEW(PgfCncCat),
	PGF_READ_NEW(PgfConcr),
	);

static PgfReader*
pgf_new_reader(GuIn* in, GuPool* opool, GuPool* pool, GuExn* err)
{
	PgfReader* rdr = gu_new(PgfReader, pool);
	rdr->opool = opool;
	rdr->symtab = gu_new_symtable(opool, pool);
	rdr->err = err;
	rdr->in = in;
	rdr->curr_sequences = gu_null_seq;
	rdr->curr_cncfuns = gu_null_seq;
	rdr->read_to_map = gu_new_type_map(&pgf_read_to_table, pool);
	rdr->read_new_map = gu_new_type_map(&pgf_read_new_table, pool);
	rdr->pool = pool;
	rdr->ctx = gu_new_addr_map(GuType, void*, &gu_null, pool);
	return rdr;
}


PgfPGF*
pgf_read_pgf(GuIn* in, GuPool* pool, GuExn* err)
{
	GuPool* tmp_pool = gu_new_pool();
	PgfReader* rdr = pgf_new_reader(in, pool, tmp_pool, err);
	PgfPGF* pgf = pgf_read_new(rdr, gu_type(PgfPGF), pool);
	gu_pool_free(tmp_pool);
	gu_return_on_exn(err, NULL);
	return pgf;
}
