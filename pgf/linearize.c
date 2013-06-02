// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

#include "data.h"
#include "linearize.h"
#include <gu/map.h>
#include <gu/fun.h>
#include <gu/log.h>
#include <gu/choice.h>
#include <gu/seq.h>
#include <gu/string.h>
#include <gu/assert.h>
#include <gu/generic.h>
#include <pgf/expr.h>

typedef GuStringMap PgfLinInfer;
typedef GuSeq PgfProdSeq;

static GU_DEFINE_TYPE(PgfProdSeq, GuSeq, gu_type(PgfProduction));

typedef struct PgfLinInferEntry PgfLinInferEntry;

struct PgfLinInferEntry {
	PgfCCat* cat;
	PgfCncFun* fun;
};

static GU_DEFINE_TYPE(
	PgfLinInferEntry, struct,
	GU_MEMBER_P(PgfLinInferEntry, cat, PgfCCat)
	// ,GU_MEMBER(PgfLinInferEntry, fun, ...)
	);

typedef GuBuf PgfLinInfers;
static GU_DEFINE_TYPE(PgfLinInfers, GuBuf, gu_type(PgfLinInferEntry));

typedef GuMap PgfCncProds;
static GU_DEFINE_TYPE(PgfCncProds, GuMap,
		      gu_type(int32_t), gu_int32_hasher,
		      gu_type(PgfProdSeq), &gu_null_seq);

typedef GuStringMap PgfLinProds;
static GU_DEFINE_TYPE(PgfLinProds, GuStringMap, gu_ptr_type(PgfCncProds),
		      &gu_null_struct);


typedef GuMap PgfInferMap;
static GU_DEFINE_TYPE(PgfInferMap, GuMap,
		      gu_type(PgfCCatIds), NULL,
		      gu_ptr_type(PgfLinInfers), &gu_null_struct);

typedef GuStringMap PgfFunIndices;
static GU_DEFINE_TYPE(PgfFunIndices, GuStringMap, gu_ptr_type(PgfInferMap),
		      &gu_null_struct);

typedef GuBuf PgfCCatBuf;
static GU_DEFINE_TYPE(PgfCCatBuf, GuBuf, gu_ptr_type(PgfCCat));

typedef GuMap PgfCoerceIdx;
static GU_DEFINE_TYPE(PgfCoerceIdx, GuMap,
		      gu_type(PgfCCat), NULL,
		      gu_ptr_type(PgfCCatBuf), &gu_null_struct);

struct PgfLzr {
	PgfConcr* cnc;
	GuPool* pool;
	GuHasher* ccat_ids_hasher;
	PgfFunIndices* fun_indices;
	PgfCoerceIdx* coerce_idx;
};

GU_DEFINE_TYPE(
	PgfLzr, struct,
	GU_MEMBER_P(PgfLzr, cnc, PgfConcr),
	GU_MEMBER_P(PgfLzr, fun_indices, PgfFunIndices),
	GU_MEMBER_P(PgfLzr, coerce_idx, PgfCoerceIdx));




static void
pgf_lzr_add_infer_entry(PgfLzr* lzr,
			PgfInferMap* infer_table,
			PgfCCat* cat,
			PgfProductionApply* papply)
{
	PgfPArgs args = papply->args;
	size_t n_args = gu_seq_length(args);
	PgfCCatIds arg_cats = gu_new_seq(PgfCCatId, n_args, lzr->pool);
	for (size_t i = 0; i < n_args; i++) {
		// XXX: What about the hypos in the args?
		gu_seq_set(arg_cats, PgfCCatId, i, gu_seq_get(args, PgfPArg, i).ccat);
	}
	gu_debug("%d,%d,%d -> %d, %s",
		 n_args > 0 ? gu_seq_get(arg_cats, PgfCCatId, 0)->fid : -1,
		 n_args > 1 ? gu_seq_get(arg_cats, PgfCCatId, 1)->fid : -1,
		 n_args > 2 ? gu_seq_get(arg_cats, PgfCCatId, 2)->fid : -1,
		 cat->fid, papply->fun->fun);
	PgfLinInfers* entries =
		gu_map_get(infer_table, &arg_cats, PgfLinInfers*);
	if (!entries) {
		entries = gu_new_buf(PgfLinInferEntry, lzr->pool);
		gu_map_put(infer_table, &arg_cats, PgfLinInfers*, entries);
	} else {
		// XXX: arg_cats is duplicate, we ought to free it
		// Display warning?
	}

	PgfLinInferEntry entry = {
		.cat = cat,
		.fun = papply->fun
	};
	gu_buf_push(entries, PgfLinInferEntry, entry);
}
			

static void
pgf_lzr_index(PgfLzr* lzr, PgfCCat* cat, PgfProduction prod)
{
	void* data = gu_variant_data(prod);
	switch (gu_variant_tag(prod)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		PgfInferMap* infer =
			gu_map_get(lzr->fun_indices, &papply->fun->fun,
				   PgfInferMap*);
		gu_debug("index: %s -> %d", papply->fun->fun, cat->fid);
		if (!infer) {
			infer = gu_new_map(PgfCCatIds*, lzr->ccat_ids_hasher,
					   PgfLinInfers*, &gu_null_struct,
					   lzr->pool);
			gu_map_put(lzr->fun_indices,
				   &papply->fun->fun, PgfInferMap*, infer);
		}
		pgf_lzr_add_infer_entry(lzr, infer, cat, papply);
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = data;
		PgfCCatBuf* cats = gu_map_get(lzr->coerce_idx, pcoerce->coerce,
					      PgfCCatBuf*);
		if (!cats) {
			cats = gu_new_buf(PgfCCat*, lzr->pool);
			gu_map_put(lzr->coerce_idx, 
				   pcoerce->coerce, PgfCCatBuf*, cats);
		}
		gu_debug("coerce_idx: %d -> %d", pcoerce->coerce->fid, cat->fid);
		gu_buf_push(cats, PgfCCat*, cat);
		break;
	}
	default:
		// Display warning?
		break;
	}
}

static void
pgf_lzr_index_ccat(PgfLzr* lzr, PgfCCat* cat)
{
	gu_debug("ccat: %d", cat->fid);
	if (gu_seq_is_null(cat->prods)) {
		return;
	}
	size_t n_prods = gu_seq_length(cat->prods);
	for (size_t i = 0; i < n_prods; i++) {
		PgfProduction prod = gu_seq_get(cat->prods, PgfProduction, i);
		pgf_lzr_index(lzr, cat, prod);
	}
}

typedef struct {
	GuMapItor fn;
	PgfLzr* lzr;
} PgfLzrIndexFn;

static void
pgf_lzr_index_cnccat_cb(GuMapItor* fn, const void* key, void* value,
			GuExn* err)
{
	PgfLzrIndexFn* clo = (PgfLzrIndexFn*) fn;
	PgfCncCat** cnccatp = value;
	PgfCncCat* cnccat = *cnccatp;
	gu_enter("-> cnccat: %s", cnccat->cid);
	size_t n_ccats = gu_seq_length(cnccat->cats);
	for (size_t i = 0; i < n_ccats; i++) {
		PgfCCat* cat = gu_seq_get(cnccat->cats, PgfCCatId, i);
		if (cat) {
			pgf_lzr_index_ccat(clo->lzr, cat);
		}
	}
	gu_exit("<-");
}


PgfLzr*
pgf_new_lzr(PgfConcr* cnc, GuPool* pool)
{
	PgfLzr* lzr = gu_new(PgfLzr, pool);
	lzr->cnc = cnc;
	lzr->pool = pool;
	lzr->fun_indices = gu_map_type_new(PgfFunIndices, pool);
	lzr->coerce_idx = gu_map_type_new(PgfCoerceIdx, pool);

	// XXX: get maybe instantiate from typetable directly?
	GuPool* tmp_pool = gu_local_pool();
	GuGeneric* genhash = gu_new_generic(gu_hasher_instances, tmp_pool);
	lzr->ccat_ids_hasher = gu_specialize(genhash, gu_type(PgfCCatIds), pool);
	gu_pool_free(tmp_pool);

	PgfLzrIndexFn clo = { { pgf_lzr_index_cnccat_cb }, lzr };
	gu_map_iter(cnc->cnccats, &clo.fn, gu_null_exn());
	size_t n_extras = gu_seq_length(cnc->extra_ccats);
	for (size_t i = 0; i < n_extras; i++) {
		PgfCCat* cat = gu_seq_get(cnc->extra_ccats, PgfCCat*, i);
		pgf_lzr_index_ccat(lzr, cat);
	}
	// TODO: prune productions with zero linearizations
	return lzr;
}

typedef struct PgfLzn PgfLzn;

struct PgfLzn {
	PgfLzr* lzr;
	GuChoice* ch;
	PgfExpr expr;
	GuEnum en;
};


//
// PgfCncTree
//

typedef GuSeq PgfCncTrees;

typedef enum {
	PGF_CNC_TREE_APP,
	PGF_CNC_TREE_LIT,
} PgfCncTreeTag;

typedef struct PgfCncTreeApp PgfCncTreeApp;
struct PgfCncTreeApp {
	PgfCncFun* fun;
	PgfCncTrees args;
};

typedef struct PgfCncTreeLit PgfCncTreeLit;
struct PgfCncTreeLit {
	PgfLiteral lit;
};


static PgfCCat*
pgf_lzn_pick_supercat(PgfLzn* lzn, PgfCCat* cat)
{
	gu_enter("->");
	while (true) {
		PgfCCatBuf* supers =
			gu_map_get(lzn->lzr->coerce_idx, cat, PgfCCatBuf*);
		if (!supers) {
			break;
		}
		gu_debug("n_supers: %d", gu_buf_length(supers));
		int ch = gu_choice_next(lzn->ch, gu_buf_length(supers) + 1);
		gu_debug("choice: %d", ch);
		if (ch == 0) {
			break;
		}
		cat = gu_buf_get(supers, PgfCCat*, ch - 1);
	}
	gu_exit("<- %d", pgf_ccat_fid(cat));
	return cat;
}

static PgfCCat*
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfCncTree* ctree_out);

static PgfCCat*
pgf_lzn_infer_apply_try(PgfLzn* lzn, GuSeq args,
			PgfInferMap* infer, GuChoiceMark* marks,
			PgfCCatIds arg_cats, int* ip, 
			GuPool* pool, PgfCncTreeApp* app_out)
{
	// gu_enter("f: %s, *ip: %d, n_args: %d", appl->fun, *ip, n_args);
	int n_args = gu_seq_length(args);
	gu_enter("->");
	PgfCCat* ret = NULL;
	while (*ip < n_args) {
		PgfCncTree* arg_treep = NULL;
		if (app_out) {
			arg_treep = gu_seq_index(app_out->args, PgfCncTree, *ip);
		}
		PgfExpr arg_expr = gu_seq_get(args, PgfExpr, *ip);
		PgfCCat* arg_i =
			pgf_lzn_infer(lzn, arg_expr, pool, arg_treep);
		if (arg_i == NULL) {
			goto finish;
		}
		arg_i = pgf_lzn_pick_supercat(lzn, arg_i);
		gu_seq_set(arg_cats, PgfCCatId, *ip, arg_i);
		marks[++*ip] = gu_choice_mark(lzn->ch);
	}
	PgfLinInfers* entries = gu_map_get(infer, &arg_cats, PgfLinInfers*);
	if (!entries) {
		goto finish;
	}
	size_t n_entries = gu_buf_length(entries);
	int e = gu_choice_next(lzn->ch, n_entries);
	gu_debug("entry %d of %d", e, n_entries);
	if (e < 0) {
		goto finish;
	}
	PgfLinInferEntry* entry = gu_buf_index(entries, PgfLinInferEntry, e);
	if (app_out != NULL) {
		app_out->fun = entry->fun;
	}
	ret = entry->cat;
finish:
	gu_exit("fid: %d", ret ? ret->fid : -1);
	return ret;
}
typedef GuSeq PgfChoiceMarks;

static PgfCCat*
pgf_lzn_infer_application(PgfLzn* lzn, PgfCId fun, GuSeq args,
			  GuPool* pool, PgfCncTree* ctree_out)
{
	PgfInferMap* infer =
		gu_map_get(lzn->lzr->fun_indices, &fun, PgfInferMap*);
	size_t n_args = gu_seq_length(args);
	gu_enter("->");
	if (infer == NULL) {
		gu_exit("<- couldn't find f");
		return NULL;
	}
	GuPool* tmp_pool = gu_new_pool();
	PgfCCat* ret = NULL;
	PgfCCatIds arg_cats = gu_new_seq(PgfCCatId, n_args, tmp_pool);

	PgfCncTreeApp* appt = NULL;
	if (ctree_out) {
		appt = gu_new_variant(PGF_CNC_TREE_APP, PgfCncTreeApp, 
				      ctree_out, pool);
		appt->args = gu_new_seq(PgfCncTree, n_args, pool);
	}

	PgfChoiceMarks marks =
		gu_new_seq(GuChoiceMark, n_args + 1, tmp_pool);
	GuChoiceMark* markdata = gu_seq_data(marks);
	int i = 0;
	markdata[i] = gu_choice_mark(lzn->ch);
	while (true) {
		ret = pgf_lzn_infer_apply_try(
			lzn, args, infer, markdata, arg_cats, &i, pool, appt);
		if (ret != NULL) {
			break;
		}

		do {
			--i;
			if (i < 0) {
				goto finish;
			}
			gu_choice_reset(lzn->ch, markdata[i]);
		} while (!gu_choice_advance(lzn->ch));
	}
finish:
	gu_pool_free(tmp_pool);
	gu_exit("<- fid: %d", ret ? ret->fid : -1);
	return ret;
}

static PgfCCat*
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfCncTree* ctree_out)
{
	PgfCCat* ret = NULL;
	GuPool* tmp_pool = gu_local_pool();
	PgfApplication* appl = pgf_expr_unapply(expr, tmp_pool);
	if (appl != NULL) {
		PgfExprFun* fun = gu_variant_data(appl->fun);
		ret = pgf_lzn_infer_application(
			lzn, fun->fun, appl->args, pool, ctree_out);
	} else {
		GuVariantInfo i = gu_variant_open(pgf_expr_unwrap(expr));
		switch (i.tag) {
		case PGF_EXPR_LIT: {
			PgfExprLit* elit = i.data;
			if (pool != NULL) {
				*ctree_out = gu_new_variant_i(
					pool, PGF_CNC_TREE_LIT,
					PgfCncTreeLit,
					.lit = elit->lit);
			}
			ret = pgf_literal_cat(elit->lit);
		}
		default:
			// XXX: should we do something here?
			break;
		}
	}
	gu_pool_free(tmp_pool);
	return ret;
}


static PgfCncTree
pgf_lzn_next(PgfLzn* lzn, GuPool* pool)
{
	// XXX: rewrite this whole mess
	PgfCncTree ctree = gu_null_variant;
	if (gu_variant_is_null(lzn->expr)) {
		return ctree;
	}
	PgfCCat* cat = NULL;
	GuChoiceMark mark = gu_choice_mark(lzn->ch);
	do {
		cat = pgf_lzn_infer(lzn, lzn->expr, NULL, NULL);
		gu_choice_reset(lzn->ch, mark);
	} while (!cat && gu_choice_advance(lzn->ch));

	if (cat) {
		PgfCCat* cat2 = pgf_lzn_infer(lzn, lzn->expr, pool, &ctree);
		gu_assert(cat == cat2);
		gu_debug("fid: %d", cat->fid);
		gu_choice_reset(lzn->ch, mark);
		if (!gu_choice_advance(lzn->ch)) {
			lzn->expr = gu_null_variant;
		}
	}
	return ctree;
}

static bool
pgf_cnc_tree_enum_next(GuEnum* self, void* to, GuPool* pool)
{
	PgfLzn* lzn = gu_container(self, PgfLzn, en);
	PgfCncTree tree = pgf_lzn_next(lzn, pool);
	if (gu_variant_is_null(tree)) {
		return false;
	}
	PgfCncTree* toc = to;
	*toc = tree;
	return true;
}

PgfCncTreeEnum*
pgf_lzr_concretize(PgfLzr* lzr, PgfExpr expr, GuPool* pool)
{
	PgfLzn* lzn = gu_new(PgfLzn, pool);
	lzn->lzr = lzr;
	lzn->expr = expr;
	lzn->ch = gu_new_choice(pool);
	lzn->en.next = pgf_cnc_tree_enum_next;
	return &lzn->en;
}


int
pgf_cnc_tree_n_ctnts(PgfCncTree ctree)
{
	GuVariantInfo cti = gu_variant_open(ctree);
	switch (cti.tag) {
	case PGF_CNC_TREE_LIT: 
		return 1;
	case PGF_CNC_TREE_APP: {
		PgfCncTreeApp* fapp = cti.data;
		return (int) gu_seq_length(fapp->fun->lins);
	}
	default:
		gu_impossible();
		return 0;
	}
}

void
pgf_lzr_linearize(PgfLzr* lzr, PgfCncTree ctree, PgfCtntId lin_idx,
		  PgfPresenter* prtr)
{
	GuVariantInfo cti = gu_variant_open(ctree);
	gu_require(lin_idx >= 0);
	switch (cti.tag) {
	case PGF_CNC_TREE_LIT: {
		gu_require(lin_idx == 0);
		PgfCncTreeLit* flit = cti.data;
		gu_invoke_maybe(0, prtr, expr_literal, flit->lit);
		break;
	}
	case PGF_CNC_TREE_APP: {
		PgfCncTreeApp* fapp = cti.data;
		PgfCncFun* fun = fapp->fun;
		size_t n_args = gu_seq_length(fapp->args);
		gu_invoke_maybe(0, prtr, expr_apply, fun->fun, n_args);
		gu_require((size_t) lin_idx < gu_seq_length(fun->lins));
		PgfSequence seq = gu_seq_get(fun->lins, PgfSequence, lin_idx);
		size_t nsyms = gu_seq_length(seq);
		PgfSymbol* syms = gu_seq_data(seq);
		for (size_t i = 0; i < nsyms; i++) {
			PgfSymbol sym = syms[i];
			GuVariantInfo sym_i = gu_variant_open(sym);
			switch (sym_i.tag) {
			case PGF_SYMBOL_CAT:
			case PGF_SYMBOL_VAR:
			case PGF_SYMBOL_LIT: {
				PgfSymbolIdx* sidx = sym_i.data;
				PgfCncTree argf = gu_seq_get(fapp->args,
							     PgfCncTree,
							     sidx->d);
				pgf_lzr_linearize(lzr, argf, sidx->r, prtr);
				break;
			}
			case PGF_SYMBOL_KS: {
				PgfSymbolKS* ks = sym_i.data;
				gu_invoke_maybe(0, prtr, symbol_tokens, ks->tokens);
				break;
			}
			case PGF_SYMBOL_KP: {
				// TODO: correct prefix-dependencies
				PgfSymbolKP* kp = sym_i.data;
				gu_invoke_maybe(0, prtr, symbol_tokens,
						kp->default_form);
				break;
			}
			default:
				gu_impossible(); 
			}
		}
		break;
	} // case PGF_CNC_TREE_APP
	default:
		gu_impossible();
	}
}



typedef struct PgfSimplePrtr PgfSimplePrtr;

struct PgfSimplePrtr {
	PgfPresenter prtr;
	GuWriter* wtr;
	GuExn* err;
};

static void
pgf_simple_prtr_symbol_tokens(PgfPresenter* prtr, PgfTokens toks)
{
	PgfSimplePrtr* flin = gu_container(prtr, PgfSimplePrtr, prtr);
	if (!gu_ok(flin->err)) {
		return;
	}
	size_t len = gu_seq_length(toks);
	for (size_t i = 0; i < len; i++) {
		PgfToken tok = gu_seq_get(toks, PgfToken, i);
		gu_string_write(tok, flin->wtr, flin->err);
		gu_putc(' ', flin->wtr, flin->err);
	}
}

static PgfPresenterFuns pgf_simple_prtr_funs = {
	.symbol_tokens = pgf_simple_prtr_symbol_tokens
};

void
pgf_lzr_linearize_simple(PgfLzr* lzr, PgfCncTree ctree,
			 PgfCtntId lin_idx, GuWriter* wtr, GuExn* err)
{
	PgfSimplePrtr flin = {
		.prtr.funs = &pgf_simple_prtr_funs,
		.wtr = wtr,
		.err = err
	};
	pgf_lzr_linearize(lzr, ctree, lin_idx, &flin.prtr);
}
