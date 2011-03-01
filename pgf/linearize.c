/* 
 * Copyright 2010 University of Helsinki.
 *   
 * This file is part of libpgf.
 * 
 * Libpgf is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * Libpgf is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libpgf. If not, see <http://www.gnu.org/licenses/>.
 */

#include "data.h"
#include "linearize.h"
#include <gu/map.h>
#include <gu/fun.h>
#include <pgf/expr.h>

// Type and dump for GPtrArray

typedef const struct PgfGPtrArrayType PgfGPtrArrayType, GuType_GPtrArray;

struct PgfGPtrArrayType {
	GuType_abstract abstract_base;
	GuType* elem_type;
};

#define GU_TYPE_INIT_GPtrArray(k_, t_, elem_type_) {	   \
	.abstract_base = GU_TYPE_INIT_abstract(k_, t_, _), \
	.elem_type = elem_type_,		   	\
}

GU_DEFINE_KIND(GPtrArray, abstract);

static void
pgf_dump_gptrarray(GuDumpFn* dumper, GuType* type, const void* p, 
		   GuDumpCtx* ctx)
{
	(void) dumper;
	PgfGPtrArrayType* atype = gu_type_cast(type, GPtrArray);
	const GPtrArray* arr = p;
	gu_yaml_begin_sequence(ctx->yaml);
	for (int i = 0; i < (int) arr->len; i++) {
		gu_dump(atype->elem_type, &arr->pdata[i], ctx);
	}
	gu_yaml_end(ctx->yaml);
}

GuTypeTable
pgf_linearize_dump_table = GU_TYPETABLE(
	GU_SLIST_0,
	{ gu_kind(GPtrArray), gu_fn(pgf_dump_gptrarray) },
	);


typedef GuStringMap PgfLinProds;
typedef GuStringMap PgfLinInfer;

GU_DEFINE_TYPE(
	PgfLinProds, GuStringMap,
	GU_TYPE_LIT(pointer, GuIntMap*,
	GU_TYPE_LIT(GuIntMap, GuIntMap,
	GU_TYPE_LIT(pointer, GPtrArray*,
	GU_TYPE_LIT(GPtrArray, GPtrArray, 
	GU_TYPE_LIT(GuVariantAsPtr,
	gu_type(PgfProduction)))))));

typedef struct PgfLinIndex PgfLinIndex;

struct PgfLinIndex {
	GuIntMap* prods; // FId |-> GPtrArray [PgfProduction]
	GuMap* infer; // [CId...] |-> GPtrArray [CId]
};

struct PgfLinearizer {
	PgfPGF* pgf;
	PgfConcr* cnc;
	GuPool* pool;
	GuStringMap* fun_indices; // PgfCId |-> PgfLinIndex
};

typedef PgfLinearizer PgfLzr;

GU_DEFINE_TYPE(
	PgfLinearizer, struct,
	GU_MEMBER_P(PgfLzr, pgf, PgfPGF),
	GU_MEMBER_P(PgfLzr, cnc, PgfConcr),
//	GU_MEMBER_P(PgfLzr, prods, PgfLinProds)
	);
 
/*
GU_DEFINE_TYPE(
	PgfLinearizer, struct,
	GU_MEMBER_P(PgfLinearizer, pgf, PgfPGF),
	GU_MEMBER_P(PgfLinearizer, cnc, PgfConcr),
	GU_MEMBER_V(PgfLinearizer, 
*/

static void
pgf_lzr_parr_free_cb(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	GPtrArray* parr = clo->env1;
	g_ptr_array_free(parr, TRUE);
}

GuFn pgf_lzr_parr_free_fn = pgf_lzr_parr_free_cb;

static GPtrArray*
pgf_lzr_parr_new(GuPool* pool)
{
	GPtrArray* parr = g_ptr_array_new();
	GuClo1* clo = gu_new_s(pool, GuClo1, 
			       pgf_lzr_parr_free_cb, parr);
	gu_pool_finally(pool, &clo->fn);
	return parr;
}


static void
pgf_lzr_arr_free_cb(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	GArray* parr = clo->env1;
	g_array_free(parr, TRUE);
}

GuFn pgf_lzr_arr_free_fn = pgf_lzr_arr_free_cb;

static GArray*
pgf_lzr_arr_new(GuPool* pool, size_t elem_size)
{
	GArray* arr = g_array_new(FALSE, FALSE, elem_size);
	GuClo1* clo = gu_new_s(pool, GuClo1, 
			       pgf_lzr_arr_free_cb, arr);
	gu_pool_finally(pool, &clo->fn);
	return arr;
}



static unsigned
pgf_lzr_pargs_hash(gconstpointer p)
{
	const PgfPArgs* pargs = p;
	int len = gu_list_length(pargs);
	unsigned h = 0;
	for (int i = 0; i < len; i++) {
		PgfPArg* parg = gu_list_elems(pargs)[i];
		h = gu_hash_mix(h, parg->fid);
		for (int j = 0; j < parg->n_hypos; j++) {
			h = gu_hash_mix(h, parg->hypos[j]);
		}
	}
	return h;
}

static gboolean
pgf_lzr_pargs_equal(gconstpointer p1, gconstpointer p2)
{
	const PgfPArgs* pargs1 = p1;
	const PgfPArgs* pargs2 = p2;
	int len = gu_list_length(pargs1);
	if (gu_list_length(pargs2) != len) {
		return FALSE;
	}
	for (int i = 0; i < len; i++) {
		PgfPArg* parg1 = gu_list_elems(pargs1)[i];
		PgfPArg* parg2 = gu_list_elems(pargs2)[i];
		if (parg1->fid != parg2->fid) {
			return FALSE;
		}
		if (parg1->n_hypos != parg2->n_hypos) {
			return FALSE;
		}
		for (int j = 0; j < parg1->n_hypos; j++) {
			if (parg1->hypos[j] != parg2->hypos[j]) {
				return FALSE;
			}
		}
	}
	return TRUE;
}


static unsigned
pgf_lzr_fids_hash(gconstpointer p)
{
	const PgfFIds* fids = p;
	int len = gu_list_length(fids);
	unsigned h = 0;
	for (int i = 0; i < len; i++) {
		h = gu_hash_mix(h, gu_list_elems(fids)[i]);
	}
	return h;
}

static gboolean
pgf_lzr_fids_equal(gconstpointer p1, gconstpointer p2)
{
	const PgfFIds* fids1 = p1;
	const PgfFIds* fids2 = p2;
	int len = gu_list_length(fids1);
	if (gu_list_length(fids2) != len) {
		return FALSE;
	}
	for (int i = 0; i < len; i++) {
		PgfFId fid1 = gu_list_elems(fids1)[i];
		PgfFId fid2 = gu_list_elems(fids2)[i];
		if (fid1 != fid2) {
			return FALSE;
		}
	}
	return TRUE;
}

typedef struct PgfLinInferEntry PgfLinInferEntry;

struct PgfLinInferEntry {
	PgfFId cat_id;
	PgfFId fun_id;
};

static void
pgf_lzr_add_infer_entry(PgfLinearizer* lzr,
			GuMap* infer_table,
			PgfFId fid,
			PgfProductionApply* papply)
{
	// XXX: it's silly we have to allocate and copy this. TODO:
	// make PgfProductionApply have PgfPArgs directly.
	PgfFIds* arg_fids =
		gu_list_new(PgfFIds, lzr->pool, papply->n_args);
	for (int i = 0; i < papply->n_args; i++) {
		// XXX: What about the hypos in the args?
		gu_list_elems(arg_fids)[i] = papply->args[i]->fid;
	}
	GArray* entries = gu_map_get(infer_table, arg_fids);
	if (entries == NULL) {
		infer_fids = pgf_lzr_arr_new(lzr->pool,
					     sizeof(PgfLinInferEntry));
		gu_map_set(infer_table, arg_fids, infer_fids);
	} else {
		// XXX: arg_fids is duplicate, we ought to free it
	}

	PgfLinInferEntry entry = {
		.cat_id = fid,
		.fun_id = papply->fun,
	};
	g_array_append_val(entries, entry);
}
			

static void
pgf_lzr_add_linprods_entry(PgfLinearizer* lzr,
			   GuIntMap* linprods,
			   PgfFId fid,
			   PgfProduction prod)
{
	GPtrArray* prods = gu_intmap_get(linprods, fid);
	if (prods == NULL) {
		prods = pgf_lzr_parr_new(lzr->pool);
		gu_intmap_set(linprods, fid, prods);
	}
	g_ptr_array_add(prods, gu_variant_to_ptr(prod));
}


static void
pgf_lzr_index(PgfLinearizer* lzr, PgfFId fid,
	      PgfProduction prod, PgfProduction from)
{
	void* data = gu_variant_data(from);
	switch (gu_variant_tag(from)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		PgfLinIndex* idx =
			gu_map_get(lzr->fun_indices, papply->fun[0]->fun);
		if (idx == NULL) {
			idx = gu_new(lzr->pool, PgfLinIndex);
			idx->prods = gu_intmap_new(lzr->pool);
			idx->infer = gu_map_new(lzr->pool,
						pgf_lzr_fids_hash,
						pgf_lzr_fids_equal);
			gu_map_set(lzr->fun_indices,
				   papply->fun[0]->fun, idx);
		}

		pgf_lzr_add_linprods_entry(lzr, idx->prods, fid, prod);
		// XXX: we are duplicating the same entries for
		// all C -> _C, TODO: add a reverse _C -> C index
		pgf_lzr_add_infer_entry(lzr, idx->infer, fid, papply);
		return;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = data;
		PgfProductions* c_prods = gu_intmap_get(lzr->cnc->productions,
							pcoerce->coerce);
		if (c_prods == NULL) {
			return;
		}
		for (int i = 0; i < c_prods->len; i++) {
			pgf_lzr_index(lzr, fid, prod, c_prods->elems[i]);
		}
		return;
	}
	default:
		return;
	}
}

static void
pgf_lzr_create_index_cb(void* key, void* value, void* user_data)
{
	PgfFId fid = GPOINTER_TO_INT(key);
	PgfProductions* prods = value;
	PgfLinearizer* lzr = user_data;

	for (int i = 0; i < prods->len; i++) {
		PgfProduction prod = prods->elems[i];
		pgf_lzr_index(lzr, fid, prod, prod);
	}
}





PgfLinearizer*
pgf_linearizer_new(GuPool* pool, PgfPGF* pgf, PgfConcr* cnc)
{
	PgfLinearizer* lzr = gu_new(pool, PgfLinearizer);
	lzr->pgf = pgf;
	lzr->cnc = cnc;
	lzr->pool = pool;
	lzr->fun_indices = gu_stringmap_new(pool);
	g_hash_table_foreach(cnc->productions, pgf_lzr_create_index_cb, lzr);
	// TODO: prune productions with zero linearizations
	return lzr;
}

struct PgfLinearization {
	PgfLinearizer* lzr;
	GByteArray* path;
	PgfExpr expr;
	PgfFIds* fids;
};

typedef PgfLinearization PgfLzn;


//
// PgfLinTree
//

typedef GuVariant PgfLinTree;

typedef enum {
	PGF_LIN_TREE_APP,
	PGF_LIN_TREE_LIT,
} PgfLinTreeTag;

typedef struct PgfLinTreeApp PgfLinTreeApp;
struct PgfLinTreeApp {
	PgfFId fun_id;
	GuLength n_args;
	PgfLinTree args[];
};

typedef struct PgfLinTreeLit PgfLinTreeLit;
struct PgfLinTreeLit {
	PgfLiteral lit;
};

typedef struct PgfLinInfer PgfLinInfer;

struct PgfLinInfer {
	PgfFId c_id;
	PgfLinTree tree;
};

static PgfLinInfer
pgf_lzc_infer_arg(PgfLzc* lzc, PgfApplication* appl, PgfLinInfer* args,
		  int argno, GuPool* pool)
{
	if (argno == appl->n_args) {
		
		
}

static PgfLinInfer
pgf_lzc_infer_application(PgfLzc* lzc, PgfApplication* appl, GuPool* pool)
{
	PgfLinInfer ret = { PGF_FID_INVALID, gu_variant_null };
	PgfLinIndex* idx = gu_map_get(lzr->fun_indices, appl->fun);
	if (idx == NULL) {
		return ret;
	}
		
	int n = appl->n_args;

	PgfLinInfer args[n];
	
	for (int i = 0; i < n; i++) {
		args[i] = pgf_lzc_infer(lzc, appl->args[i], pool);
	}
	
	
	int mods[n];
	int j = 1;
	for (int i = 0; i < n; i++) {
			mods[i] = j;
			j = j * (int) arrs[i]->len;
	}
	PgfFIds* arg_fids = gu_list_new(PgfFIds, pool, n);
	while (j > 0) {
		for (int i = 0; i < n; i++) {
				GArray* a = arrs[i];
				int idx = (j / mods[i]) % (int) a->len;
				gu_list_elems(arg_fids)[i] =
					g_array_index(a, PgfFId, idx);
			}
			GArray* res_fids = gu_map_get(idx->infer, arg_fids);
			if (res_fids != NULL) {
				g_array_append_vals(fids_out,
						    (PgfFId*) res_fids->data,
						    res_fids->len);
			}
			j--;
		}
}

static PgfLinInfer
pgf_lzc_infer(PgfLzc* lzc, PgfExpr expr, GuPool* pool)
{
	GuPool* tmp_pool = gu_pool_new();
	PgfApplication* appl = pgf_expr_unapply(expr, tmp_pool);
	PgfLinInfer ret = { .c_id = PGF_FID_INVALID,
			    .tree = gu_variant_null };
	if (appl != NULL) {
		ret = pgf_lzc_infer_application(lzc, expr, appl);
	} else {
		GuVariantInfo i = gu_variant_open(pgf_expr_unwrap(expr));
		switch (i.tag) {
		case PGF_EXPR_LIT: {
			PgfExprLit* elit = i.data;
			if (pool != NULL) {
				ret.tree = gu_variant_new_s(
					pool, PGF_LIN_TREE_LIT,
					.lit = elit->lit);
			}
			ret.c_id = pgf_literal_fid(elit->lit);
			break;
		}
		default:
			// XXX: should we do something here?
			break;
		}
	}
	gu_pool_free(tmp_pool);

	return ret;
}



static void
pgf_lzr_infer_fids(PgfLzr* lzr, PgfExpr expr, GArray* fids_out)
{
	GuPool* pool = gu_pool_new();
	PgfApplication* appl = pgf_expr_unapply(expr, pool);
	if (appl == NULL) {
		GuVariantInfo i = gu_variant_open(pgf_expr_unwrap(expr));
		switch (i.tag) {
		case PGF_EXPR_LIT: {
			PgfExprLit* elit = i.data;
			PgfFId fid = pgf_literal_fid(elit->lit);
			g_array_append_val(fids_out, fid);
			break;
		}
		default:
			// XXX: should we do something here?
			break;
		}
		goto exit;
	}
	PgfLinIndex* idx = gu_map_get(lzr->fun_indices, appl->fun);
	if (idx != NULL) {
		int n = appl->n_args;
		GArray* arrs[n];
		for (int i = 0; i < n; i++) {
			arrs[i] = pgf_lzr_arr_new(pool, sizeof(PgfFId));
			pgf_lzr_infer_fids(lzr, appl->args[i], arrs[i]);
		}
		int mods[n];
		int j = 1;
		for (int i = 0; i < n; i++) {
			mods[i] = j;
			j = j * (int) arrs[i]->len;
		}
		PgfFIds* arg_fids = gu_list_new(PgfFIds, pool, n);
		while (j > 0) {
			for (int i = 0; i < n; i++) {
				GArray* a = arrs[i];
				int idx = (j / mods[i]) % (int) a->len;
				gu_list_elems(arg_fids)[i] =
					g_array_index(a, PgfFId, idx);
			}
			GArray* res_fids = gu_map_get(idx->infer, arg_fids);
			if (res_fids != NULL) {
				g_array_append_vals(fids_out,
						    (PgfFId*) res_fids->data,
						    res_fids->len);
			}
			j--;
		}
	}
exit:
	gu_pool_free(pool);
}


static void 
pgf_lzn_byte_array_free_cb(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	g_byte_array_unref(clo->env1);
}

PgfLzn*
pgf_lzn_new(PgfLinearizer* lzr, PgfExpr expr, GuPool* pool)
{
	PgfLzn* lzn = NULL;
	GuPool* tmp_pool = gu_pool_new();
	GArray* fids = pgf_lzr_arr_new(tmp_pool, sizeof(PgfFId));
	pgf_lzr_infer_fids(lzr, expr, fids);
	if (fids->len > 0) {
		lzn = gu_new(pool, PgfLzn);
		lzn->lzr = lzr;
		lzn->path = g_byte_array_new();
		GuClo1* clo = gu_new_s(pool, GuClo1,
				       pgf_lzn_byte_array_free_cb, lzn->path);
		gu_pool_finally(pool, &clo->fn);
		lzn->fids = gu_list_new(PgfFIds, pool, fids->len);
		for (int i = 0; i < (int) fids->len; i++) {
			gu_list_elems(lzn->fids)[i] = g_array_index(fids, PgfFId, i);
		}
		lzn->expr = expr;
	}
	gu_pool_free(tmp_pool);
	return lzn;
}


typedef struct PgfLznCtx PgfLznCtx;

typedef PgfLznCtx PgfLzc;

struct PgfLznCtx {
	PgfLzn* lzn;
	PgfExpr expr;
	GuPool* pool;
	int path_idx;
	PgfLinFuncs* cb_fns;
	void* cb_ctx;
};



static bool
pgf_lzc_linearize(PgfLzc* lzc, PgfExpr expr, PgfFId fid, int lin_idx);

static bool
pgf_lzc_apply(PgfLzc* lzc, PgfExpr src_expr, PgfFId fid,
	      int lin_idx, PgfApplication* appl);



bool
pgf_lzn_advance(PgfLzn* lzn)
{
	GByteArray* path = lzn->path;
	while (path->len > 0 && path->data[path->len - 1] == 0) {
		g_byte_array_set_size(path, path->len - 1);
	}
	if (path->len == 0) {
		return false;
	} else {
		g_assert(path->data[path->len - 1] > 0);
		path->data[path->len - 1]--;
		return true;
	}
}

int
pgf_lzc_choice_next(PgfLzc* lzc, int n_choices)
{
	g_assert(n_choices > 0);
	if (n_choices == 1) {
		return 0;
	}
	GByteArray* path = lzc->lzn->path;
	if (lzc->path_idx == (int) path->len) {
		int next_choice = n_choices - 1;
		g_assert(next_choice <= UINT8_MAX);
		uint8_t n = (uint8_t) next_choice;
		g_byte_array_append(path, &n, 1);
	}
	return n_choices - 1 - path->data[lzc->path_idx++];
}

bool
pgf_lzc_choice_reject(PgfLzc* lzc, int n_choices)
{
	gu_assert(lzc->path_idx > 0);
	if (n_choices == 1) {
		return false;
	}
	lzc->path_idx--;
	if (path->data[lzc->path_idx] == 0) {
		return false;
	} else {
		pach->data[lzc->path_idx]--;
		return true;
	}
}


static void
pgf_lzc_choose_fid_cb(GuFn* fnp, void* key, void* value) {
	GuClo3* clo = (GuClo3*) fnp;
	int* next_fid_seqno_p = clo->env1;
	int* count_p = clo->env2;
	PgfFId* ret_fid = clo->env3;
	PgfFId fid = GPOINTER_TO_INT(key);
	(void) value;
	if (*count_p == *next_fid_seqno_p) {
		*ret_fid = fid;
	}
	++*count_p;
}

static PgfFId
pgf_lzc_choose_fid(PgfLzc* lzc, PgfCId* cid)
{
	PgfLzn* lzn = lzc->lzn;
	PgfLzr* lzr = lzn->lzr;
	PgfLinIndex* idx = gu_map_get(lzr->fun_indices, cid);
	if (idx == NULL) {
		// XXX: error reporting
		return -1;
	}
	int next_fid_seqno =
		pgf_lzc_next_choice(lzc, gu_map_size(idx->prods));
	int count = 0;
	PgfFId fid = -1;
	GuClo3 clo = { pgf_lzc_choose_fid_cb, &next_fid_seqno, &count, &fid };
	gu_map_iter(idx->prods, &clo.fn);
	g_assert(fid >= 0);
	return fid;
}


// TODO: check for invalid lin_idx. For literals, only 0 is legal.
static bool
pgf_lzc_linearize(PgfLzc* lzc, PgfExpr expr, PgfFId fid, int lin_idx) 
{
	PgfExpr uexpr = pgf_expr_unwrap(expr);
	GuPool* tpool = gu_pool();
	bool succ = FALSE;
	
	PgfApplication* appl = pgf_expr_unapply(uexpr, tpool);

	if (appl != NULL) {
		succ = pgf_lzc_apply(lzc, expr, fid, lin_idx, appl);
		goto exit;
	} 

	GuVariantInfo i = gu_variant_open(uexpr);	
	switch (i.tag) {
	case PGF_EXPR_LIT: {
		PgfExprLit* elit = i.data;
		lzc->cb_fns->expr_literal(lzc->cb_ctx, elit->lit);
		succ = true;
		goto exit;
	}
	case PGF_EXPR_ABS:
	case PGF_EXPR_VAR:
	case PGF_EXPR_META:
		// XXX: TODO
		g_assert_not_reached(); 
		break;
	default:
		g_assert_not_reached();
	}
exit:
	gu_pool_free(tpool);
	return succ;
}


static PgfProductionApply*
pgf_lzc_choose_production(PgfLzc* lzc, PgfCId* cid, PgfFId fid)
{
	PgfLzn* lzn = lzc->lzn;
	PgfLzr* lzr = lzn->lzr;

	PgfLinIndex* idx = gu_map_get(lzr->fun_indices, cid);
	if (idx == NULL) {
		return NULL;
	}
	if (fid == -1) {
		fid = pgf_lzc_choose_fid(lzc, cid);
	}

	while (TRUE) {
		GPtrArray* prods = gu_intmap_get(idx->prods, fid);
		if (prods == NULL || prods->len == 0) {
			return NULL;
		}
		int idx = pgf_lzc_next_choice(lzc, prods->len);
		PgfProduction prod = gu_variant_from_ptr(prods->pdata[idx]);
		GuVariantInfo prod_i = gu_variant_open(prod);
		switch (prod_i.tag) {
		case PGF_PRODUCTION_APPLY: {
			PgfProductionApply* papply = prod_i.data;
			return papply;
		}
		case PGF_PRODUCTION_COERCE: {
			PgfProductionCoerce* pcoerce = prod_i.data;
			fid = pcoerce->coerce;
			continue;
		}
		default:
			g_assert_not_reached();
		}
	}
}

static bool
pgf_lzc_apply(PgfLzc* lzc, PgfExpr src_expr, PgfFId fid,
	      int lin_idx, PgfApplication* appl)
{
	PgfLzn* lzn = lzc->lzn;
	PgfLzr* lzr = lzn->lzr;
	PgfPGF* pgf = lzr->pgf;
	PgfLinFuncs* fns = lzc->cb_fns;

	PgfFunDecl* fdecl = gu_map_get(pgf->abstract.funs, appl->fun);
	g_assert(fdecl != NULL); // XXX: turn this into a proper check
	
	PgfProductionApply* papply = 
		pgf_lzc_choose_production(lzc, appl->fun, fid);
	if (papply == NULL) {
		return FALSE;
	}

	g_assert(papply->n_args == appl->n_args);
	
	PgfCncFun* cfun = *papply->fun;
	g_assert(lin_idx < cfun->n_lins);
	
	PgfSequence* lin = *(cfun->lins[lin_idx]);

	if (fns->expr_apply)
		fns->expr_apply(lzc->cb_ctx, fdecl, lin->len);
	
	for (int i = 0; i < lin->len; i++) {
		PgfSymbol sym = lin->elems[i];
		GuVariantInfo sym_i = gu_variant_open(sym);
		switch (sym_i.tag) {
		case PGF_SYMBOL_CAT:
		case PGF_SYMBOL_VAR:
		case PGF_SYMBOL_LIT: {
			PgfSymbolIdx* sidx = sym_i.data;
			g_assert(sidx->d < appl->n_args);
			PgfExpr arg = appl->args[sidx->d];
			if (fns->symbol_expr)
				fns->symbol_expr(lzc->cb_ctx, sidx->d,
						 arg, sidx->r);
			bool succ =
				pgf_lzc_linearize(lzc, 
						  arg,
						  papply->args[sidx->d]->fid,
						  sidx->r);
			if (!succ) {
				return FALSE;
			}
			break;
		}
		case PGF_SYMBOL_KS: {
			PgfSymbolKS* ks = sym_i.data;
			if (fns->symbol_tokens)
				fns->symbol_tokens(lzc->cb_ctx, ks->tokens);
			break;
		}
		case PGF_SYMBOL_KP:
			// XXX: To be supported
			g_assert_not_reached(); 
		default:
			g_assert_not_reached(); 
		}
	}
	
	return TRUE;
}

void
pgf_lzn_linearize(PgfLzn* lzn, int lin_idx, 
		  PgfLinFuncs* cb_fns, void* cb_ctx)
{
	PgfLzc lzc = {
		.lzn = lzn,
		.path_idx = 0,
		.cb_fns = cb_fns,
		.cb_ctx = cb_ctx,
	};
	int fid_idx = pgf_lzc_next_choice(&lzc, lzn->fids->len);
	PgfFId fid = gu_list_elems(lzn->fids)[fid_idx];
	pgf_lzc_linearize(&lzc, lzn->expr, fid, lin_idx);
}






static void
pgf_file_lzn_symbol_tokens(void* ctx, PgfTokens* toks)
{
	FILE* out = ctx;
	
	for (int i = 0; i < toks->len; i++) {
		PgfToken* tok = toks->elems[i];
		fprintf(out, GU_STRING_FMT " ", GU_STRING_FMT_ARGS(tok));
	}
}

static PgfLinFuncs pgf_file_lin_funcs = {
	.symbol_tokens = pgf_file_lzn_symbol_tokens
};

void
pgf_lzn_linearize_to_file(PgfLzn* lzn, int lin_idx, FILE* file_out)
{
	pgf_lzn_linearize(lzn, lin_idx, 
			  &pgf_file_lin_funcs, file_out);
}
