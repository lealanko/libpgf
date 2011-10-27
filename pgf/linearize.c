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
//#define GU_LOG_ENABLE

#include "data.h"
#include "linearize.h"
#include <gu/map.h>
#include <gu/fun.h>
#include <gu/log.h>
#include <gu/choice.h>
#include <gu/seq.h>
#include <gu/assert.h>
#include <pgf/expr.h>

typedef GuStrMap PgfLinInfer;

GU_SEQ_DEFINE(PgfProdSeq, pgf_prod_seq, PgfProduction);
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

GU_SEQ_DEFINE(PgfLinInfers, pgf_lin_infers, PgfLinInferEntry);
static GU_DEFINE_TYPE(PgfLinInfers, GuSeq, gu_type(PgfLinInferEntry));

typedef GuIntMap PgfCncProds;
static GU_DEFINE_TYPE(PgfCncProds, GuIntPtrMap, gu_type(PgfProdSeq));

typedef GuStrMap PgfLinProds;
static GU_DEFINE_TYPE(PgfLinProds, GuStrPtrMap, gu_type(PgfCncProds));


static unsigned
pgf_lzr_cats_hash_fn(GuHashFn* self, const void* p)
{
	(void) self;
	const PgfCCatIds* cats = p;
	int len = gu_list_length(cats);
	unsigned h = 0;
	for (int i = 0; i < len; i++) {
		h = gu_hash_mix(h, (uintptr_t) gu_list_index(cats, i));
	}
	return h;
}

static GuHashFn
pgf_lzr_cats_hash = { pgf_lzr_cats_hash_fn };

static bool
pgf_lzr_cats_eq_fn(GuEqFn* self, const void* p1, const void* p2)
{
	(void) self;
	const PgfCCatIds* cats1 = p1;
	const PgfCCatIds* cats2 = p2;
	int len = gu_list_length(cats1);
	if (gu_list_length(cats2) != len) {
		return false;
	}
	for (int i = 0; i < len; i++) {
		PgfCCat* cat1 = gu_list_index(cats1, i);
		PgfCCat* cat2 = gu_list_index(cats2, i);
		if (cat1 != cat2) {
			return false;
		}
	}
	return true;
}

static GuEqFn
pgf_lzr_cats_eq = { pgf_lzr_cats_eq_fn };

GU_MAP_DEFINE(PgfInferMap, pgf_infer_map, 
	      R, PgfCCatIds, V, PgfLinInfers, GU_SEQ_NULL, 
	      &pgf_lzr_cats_hash, &pgf_lzr_cats_eq);

static GU_DEFINE_TYPE(PgfInferMap, GuMap, 
		      &pgf_lzr_cats_hash, &pgf_lzr_cats_eq,
		      false, gu_type(PgfCCatIds), 
		      true, gu_type(PgfLinInfers), 
		      &(PgfLinInfers) { GU_SEQ_NULL });

typedef GuStrMap PgfFunIndices;
static GU_DEFINE_TYPE(PgfFunIndices, GuStrPtrMap, gu_type(PgfInferMap));

GU_MAP_DEFINE(PgfCoerceIdx, pgf_coerce_idx, R, PgfCCat, V, PgfCCatSeq, 
	      GU_SEQ_NULL, NULL, NULL);

static GU_DEFINE_TYPE(PgfCoerceIdx, GuPtrMap, NULL, NULL, 
		      gu_type(PgfCCat), gu_type(PgfCCatSeq));

struct PgfLzr {
	PgfPGF* pgf;
	PgfConcr* cnc;
	GuPool* pool;
	PgfFunIndices* fun_indices;
	PgfCoerceIdx* coerce_idx;
};

GU_DEFINE_TYPE(
	PgfLzr, struct,
	GU_MEMBER_P(PgfLzr, pgf, PgfPGF),
	GU_MEMBER_P(PgfLzr, cnc, PgfConcr),
	GU_MEMBER_P(PgfLzr, fun_indices, PgfFunIndices),
	GU_MEMBER_P(PgfLzr, coerce_idx, PgfCoerceIdx));




static void
pgf_lzr_add_infer_entry(PgfLzr* lzr,
			PgfInferMap* infer_table,
			PgfCCat* cat,
			PgfProductionApply* papply)
{
	PgfPArgs* args = papply->args;
	int n_args = gu_list_length(args);
	PgfCCatIds* arg_cats = gu_list_new(PgfCCatIds, lzr->pool, n_args);
	for (int i = 0; i < n_args; i++) {
		// XXX: What about the hypos in the args?
		gu_list_index(arg_cats, i) = gu_list_index(args, i).ccat;
	}
	gu_debug("%d,%d,%d -> %d, %s",
		 n_args > 0 ? gu_list_index(arg_cats, 0)->fid : -1,
		 n_args > 1 ? gu_list_index(arg_cats, 1)->fid : -1,
		 n_args > 2 ? gu_list_index(arg_cats, 2)->fid : -1,
		 cat->fid, papply->fun->fun);
	PgfLinInfers entries = pgf_infer_map_get(infer_table, arg_cats);
	if (pgf_lin_infers_is_null(entries)) {
		entries = pgf_lin_infers_new(lzr->pool);
		pgf_infer_map_set(infer_table, arg_cats, entries);
	} else {
		// XXX: arg_cats is duplicate, we ought to free it
		// Display warning?
	}

	PgfLinInferEntry entry = {
		.cat = cat,
		.fun = papply->fun
	};
	pgf_lin_infers_push(entries, entry);
}
			

static void
pgf_lzr_index(PgfLzr* lzr, PgfCCat* cat, PgfProduction prod)
{
	void* data = gu_variant_data(prod);
	switch (gu_variant_tag(prod)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		PgfInferMap* infer =
			gu_strmap_get(lzr->fun_indices, papply->fun->fun);
		gu_debug("index: %s -> %d", papply->fun->fun, cat->fid);
		if (infer == NULL) {
			infer = pgf_infer_map_new(lzr->pool);
			gu_strmap_set(lzr->fun_indices,
				      papply->fun->fun, infer);
		}
		pgf_lzr_add_infer_entry(lzr, infer, cat, papply);
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = data;
		PgfCCatSeq cats = pgf_coerce_idx_get(lzr->coerce_idx,
						     pcoerce->coerce);
		if (pgf_ccat_seq_is_null(cats)) {
			cats = pgf_ccat_seq_new(lzr->pool);
			pgf_coerce_idx_set(lzr->coerce_idx, 
					   pcoerce->coerce, cats);
		}
		gu_debug("coerce_idx: %d -> %d", pcoerce->coerce->fid, cat->fid);
		pgf_ccat_seq_push(cats, cat);
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
	if (pgf_production_seq_is_null(cat->prods)) {
		return;
	}
	int n_prods = pgf_production_seq_size(cat->prods);
	for (int i = 0; i < n_prods; i++) {
		PgfProduction prod = pgf_production_seq_get(cat->prods, i);
		pgf_lzr_index(lzr, cat, prod);
	}
}

typedef struct {
	GuMapIterFn fn;
	PgfLzr* lzr;
} PgfLzrIndexFn;

static void
pgf_lzr_index_cnccat_cb(GuMapIterFn* fn, const void* key, void* value)
{
	(void) key;
	PgfLzrIndexFn* clo = (PgfLzrIndexFn*) fn;
	PgfCncCat* cnccat = value;
	gu_enter("-> cnccat: %s", cnccat->cid);
	int n_ccats = gu_list_length(cnccat->cats);
	for (int i = 0; i < n_ccats; i++) {
		PgfCCat* cat = gu_list_index(cnccat->cats, i);
		if (cat) {
			pgf_lzr_index_ccat(clo->lzr, cat);
		}
	}
	gu_exit("<-");
}


PgfLzr*
pgf_lzr_new(GuPool* pool, PgfPGF* pgf, PgfConcr* cnc)
{
	PgfLzr* lzr = gu_new(pool, PgfLzr);
	lzr->pgf = pgf;
	lzr->cnc = cnc;
	lzr->pool = pool;
	lzr->fun_indices = gu_strmap_new(pool);
	lzr->coerce_idx = pgf_coerce_idx_new(pool);
	PgfLzrIndexFn clo = { { pgf_lzr_index_cnccat_cb }, lzr };
	gu_map_iter((GuMap*)cnc->cnccats, &clo.fn);
	int n_extras = pgf_ccat_seq_size(cnc->extra_ccats);
	for (int i = 0; i < n_extras; i++) {
		PgfCCat* cat = pgf_ccat_seq_get(cnc->extra_ccats, i);
		pgf_lzr_index_ccat(lzr, cat);
	}
	// TODO: prune productions with zero linearizations
	return lzr;
}

struct PgfLzn {
	PgfLzr* lzr;
	GuChoice* ch;
	PgfExpr expr;
};

//
// PgfLinForm
//

typedef enum {
	PGF_LIN_FORM_APP,
	PGF_LIN_FORM_LIT,
} PgfLinFormTag;

typedef struct PgfLinFormApp PgfLinFormApp;
struct PgfLinFormApp {
	PgfCncFun* fun;
	GuLength n_args;
	PgfLinForm args[];
};

typedef struct PgfLinFormLit PgfLinFormLit;
struct PgfLinFormLit {
	PgfLiteral lit;
};


static PgfCCat*
pgf_lzn_pick_supercat(PgfLzn* lzn, PgfCCat* cat)
{
	gu_enter("->");
	while (true) {
		PgfCCatSeq supers = pgf_coerce_idx_get(lzn->lzr->coerce_idx, cat);
		if (pgf_ccat_seq_is_null(supers)) {
			break;
		}
		gu_debug("n_supers: %d", pgf_ccat_seq_size(supers));
		int ch = gu_choice_next(lzn->ch, pgf_ccat_seq_size(supers) + 1);
		gu_debug("choice: %d", ch);
		if (ch == 0) {
			break;
		}
		cat = pgf_ccat_seq_get(supers, ch - 1);
	}
	gu_exit("<- %d", cat->fid);
	return cat;
}

static PgfCCat*
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfLinForm* form_out);

static PgfCCat*
pgf_lzn_infer_apply_try(PgfLzn* lzn, PgfApplication* appl,
			PgfInferMap* infer, GuChoiceMark* marks,
			PgfCCatIds* arg_cats, int* ip, int n_args, 
			GuPool* pool, PgfLinFormApp* app_out)
{
	gu_enter("f: %s, *ip: %d, n_args: %d", appl->fun, *ip, n_args);
	PgfCCat* ret = NULL;
	while (*ip < n_args) {
		PgfLinForm* arg_treep = 
			(app_out == NULL ? NULL : &app_out->args[*ip]);
		PgfCCat* arg_i = 
			pgf_lzn_infer(lzn, appl->args[*ip], pool, arg_treep);
		if (arg_i == NULL) {
			goto finish;
		}
		arg_i = pgf_lzn_pick_supercat(lzn, arg_i);
		gu_list_index(arg_cats, *ip) = arg_i;
		marks[++*ip] = gu_choice_mark(lzn->ch);
	}
	PgfLinInfers entries = pgf_infer_map_get(infer, arg_cats);
	if (pgf_lin_infers_is_null(entries)) {
		goto finish;
	}
	int n_entries = pgf_lin_infers_size(entries);
	int e = gu_choice_next(lzn->ch, n_entries);
	gu_debug("entry %d of %d", e, n_entries);
	if (e < 0) {
		goto finish;
	}
	PgfLinInferEntry* entry = 
		pgf_lin_infers_index(entries, e);
	if (app_out != NULL) {
		app_out->fun = entry->fun;
	}
	ret = entry->cat;
finish:
	gu_exit("fid: %d", ret ? ret->fid : -1);
	return ret;
}
typedef GuList(GuChoiceMark) PgfChoiceMarks;

static PgfCCat*
pgf_lzn_infer_application(PgfLzn* lzn, PgfApplication* appl, 
			  GuPool* pool, PgfLinForm* form_out)
{
	PgfInferMap* infer = gu_strmap_get(lzn->lzr->fun_indices, appl->fun);
	gu_enter("-> f: %s, n_args: %d", appl->fun, appl->n_args);
	if (infer == NULL) {
		gu_exit("<- couldn't find f");
		return NULL;
	}
	GuPool* tmp_pool = gu_pool_new();
	PgfCCat* ret = NULL;
	int n = appl->n_args;
	PgfCCatIds* arg_cats = gu_list_new(PgfCCatIds, tmp_pool, n);

	PgfLinFormApp* appt = NULL;
	if (form_out) {
		appt = gu_variant_flex_new(pool, PGF_LIN_FORM_APP, PgfLinFormApp, 
					   args, n, form_out);
		appt->n_args = n;
	}

	PgfChoiceMarks* marksl = gu_list_new(PgfChoiceMarks, tmp_pool, n + 1);
	GuChoiceMark* marks = gu_list_elems(marksl);
	int i = 0;
	marks[i] = gu_choice_mark(lzn->ch);
	while (true) {
		ret = pgf_lzn_infer_apply_try(lzn, appl, infer,
					      marks, arg_cats,
					      &i, n, pool, appt);
		if (ret != NULL) {
			break;
		}

		do {
			--i;
			if (i < 0) {
				goto finish;
			}
			gu_choice_reset(lzn->ch, marks[i]);
		} while (!gu_choice_advance(lzn->ch));
	}
finish:
	gu_pool_free(tmp_pool);
	gu_exit("<- fid: %d", ret ? ret->fid : -1);
	return ret;
}

static PgfCCat*
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfLinForm* form_out)
{
	PgfCCat* ret = NULL;
	GuPool* tmp_pool = gu_pool_new();
	PgfApplication* appl = pgf_expr_unapply(expr, tmp_pool);
	if (appl != NULL) {
		ret = pgf_lzn_infer_application(lzn, appl, pool, form_out);
	} else {
		GuVariantInfo i = gu_variant_open(pgf_expr_unwrap(expr));
		switch (i.tag) {
		case PGF_EXPR_LIT: {
			PgfExprLit* elit = i.data;
			if (pool != NULL) {
				*form_out = gu_variant_new_s(
					pool, PGF_LIN_FORM_LIT,
					PgfLinFormLit,
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

PgfLzn*
pgf_lzn_new(PgfLzr* lzr, PgfExpr expr, GuPool* pool)
{
	PgfLzn* lzn = gu_new(pool, PgfLzn);
	lzn->lzr = lzr;
	lzn->expr = expr;
	lzn->ch = gu_choice_new(pool);
	return lzn;
}

PgfLinForm
pgf_lzn_next_form(PgfLzn* lzn, GuPool* pool)
{
	// XXX: rewrite this whole mess
	PgfLinForm form = gu_variant_null;
	if (gu_variant_is_null(lzn->expr)) {
		return form;
	}
	PgfCCat* cat = NULL;
	GuChoiceMark mark = gu_choice_mark(lzn->ch);
	do {
		cat = pgf_lzn_infer(lzn, lzn->expr, NULL, NULL);
		gu_choice_reset(lzn->ch, mark);
	} while (!cat && gu_choice_advance(lzn->ch));

	if (cat) {
		PgfCCat* cat2 = pgf_lzn_infer(lzn, lzn->expr, pool, &form);
		gu_assert(cat == cat2);
		gu_debug("fid: %d", cat->fid);
		gu_choice_reset(lzn->ch, mark);
		if (!gu_choice_advance(lzn->ch)) {
			lzn->expr = gu_variant_null;
		}
	}
	return form;
}


int
pgf_lin_form_n_fields(PgfLinForm form)
{
	GuVariantInfo formi = gu_variant_open(form);
	switch (formi.tag) {
	case PGF_LIN_FORM_LIT: 
		return 1;
	case PGF_LIN_FORM_APP: {
		PgfLinFormApp* fapp = formi.data;
		return fapp->fun->n_lins;
	}
	default:
		gu_impossible();
		return 0;
	}
}

void
pgf_lzr_linearize(PgfLzr* lzr, PgfLinForm form, int lin_idx, PgfLinFuncs** fnsp)
{
	PgfLinFuncs* fns = *fnsp;
	GuVariantInfo formi = gu_variant_open(form);

	switch (formi.tag) {
	case PGF_LIN_FORM_LIT: {
		gu_require(lin_idx == 0);
		PgfLinFormLit* flit = formi.data;
		if (fns->expr_literal) {
			fns->expr_literal(fnsp, flit->lit);
		}
		break;
	}
	case PGF_LIN_FORM_APP: {
		PgfLinFormApp* fapp = formi.data;
		PgfCncFun* fun = fapp->fun;
		if (fns->expr_apply) {
			fns->expr_apply(fnsp, fun->fun, fapp->n_args);
		}
		gu_require(lin_idx < fun->n_lins);
		PgfSequence* seq = fun->lins[lin_idx];
		int nsyms = gu_list_length(seq);
		PgfSymbol* syms = gu_list_elems(seq);
		for (int i = 0; i < nsyms; i++) {
			PgfSymbol sym = syms[i];
			GuVariantInfo sym_i = gu_variant_open(sym);
			switch (sym_i.tag) {
			case PGF_SYMBOL_CAT:
			case PGF_SYMBOL_VAR:
			case PGF_SYMBOL_LIT: {
				PgfSymbolIdx* sidx = sym_i.data;
				gu_assert(sidx->d < fapp->n_args);
				PgfLinForm argf = fapp->args[sidx->d];
				pgf_lzr_linearize(lzr, argf, sidx->r, fnsp);
				break;
			}
			case PGF_SYMBOL_KS: {
				PgfSymbolKS* ks = sym_i.data;
				if (fns->symbol_tokens) {
					fns->symbol_tokens(fnsp, ks->tokens);
				}
				break;
			}
			case PGF_SYMBOL_KP: {
				// TODO: correct prefix-dependencies
				PgfSymbolKP* kp = sym_i.data;
				if (fns->symbol_tokens) {
					fns->symbol_tokens(fnsp,
							   kp->default_form);
				}
				break;
			}
			default:
				gu_impossible(); 
			}
		}
		break;
	} // case PGF_LIN_FORM_APP
	default:
		gu_impossible();
	}
}



typedef struct PgfFileLin PgfFileLin;

struct PgfFileLin {
	PgfLinFuncs* funcs;
	FILE* file;
};


static void
pgf_file_lzn_symbol_tokens(PgfLinFuncs** funcs, PgfTokens* toks)
{
	PgfFileLin* flin = gu_container(funcs, PgfFileLin, funcs);
	for (int i = 0; i < toks->len; i++) {
		PgfToken tok = toks->elems[i];
		fprintf(flin->file, "%s ", tok);
	}
}

static PgfLinFuncs pgf_file_lin_funcs = {
	.symbol_tokens = pgf_file_lzn_symbol_tokens
};

void
pgf_lzr_linearize_to_file(PgfLzr* lzr, PgfLinForm form,
			  int lin_idx, FILE* file_out)
{
	PgfFileLin flin = { .funcs = &pgf_file_lin_funcs,
			    .file = file_out };
	pgf_lzr_linearize(lzr, form, lin_idx, &flin.funcs);
}
