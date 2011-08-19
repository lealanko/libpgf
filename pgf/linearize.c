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

typedef GuStringMap PgfLinInfer;

GU_SEQ_DEFINE(PgfProdSeq, pgf_prod_seq, PgfProduction);
static GU_DEFINE_TYPE(PgfProdSeq, GuSeq, gu_type(PgfProduction));

typedef struct PgfLinInferEntry PgfLinInferEntry;

struct PgfLinInferEntry {
	PgfFId cat_id;
	PgfCncFun* fun;
};

static GU_DEFINE_TYPE(
	PgfLinInferEntry, struct,
	GU_MEMBER(PgfLinInferEntry, cat_id, PgfFId)
	// ,GU_MEMBER(PgfLinInferEntry, fun, ...)
	);

GU_SEQ_DEFINE(PgfLinInfers, pgf_lin_infers, PgfLinInferEntry);
static GU_DEFINE_TYPE(PgfLinInferSeq, GuSeq, gu_type(PgfLinInferEntry));

typedef GuIntMap PgfCncProds;
static GU_DEFINE_TYPE(PgfCncProds, GuIntPtrMap, gu_type(PgfProdSeq));

typedef GuStringMap PgfLinProds;
static GU_DEFINE_TYPE(PgfLinProds, GuStringPtrMap, gu_type(PgfCncProds));


static size_t
pgf_lzr_fids_hash_fn(GuHashFn* self, const void* p)
{
	(void) self;
	const PgfFIds* fids = p;
	int len = gu_list_length(fids);
	unsigned h = 0;
	for (int i = 0; i < len; i++) {
		h = gu_hash_mix(h, gu_list_elems(fids)[i]);
	}
	return h;
}

static GuHashFn
pgf_lzr_fids_hash = { pgf_lzr_fids_hash_fn };

static bool
pgf_lzr_fids_eq_fn(GuEqFn* self, const void* p1, const void* p2)
{
	(void) self;
	const PgfFIds* fids1 = p1;
	const PgfFIds* fids2 = p2;
	int len = gu_list_length(fids1);
	if (gu_list_length(fids2) != len) {
		return false;
	}
	for (int i = 0; i < len; i++) {
		PgfFId fid1 = gu_list_elems(fids1)[i];
		PgfFId fid2 = gu_list_elems(fids2)[i];
		if (fid1 != fid2) {
			return false;
		}
	}
	return true;
}

static GuEqFn
pgf_lzr_fids_eq = { pgf_lzr_fids_eq_fn };

typedef GuMap PgfInferMap;
static GU_DEFINE_TYPE(PgfInferMap, GuPtrMap, 
		      &pgf_lzr_fids_hash, &pgf_lzr_fids_eq,
		      gu_type(PgfFIds), gu_type(PgfLinInferEntry));

typedef GuStringMap PgfFunIndices;
static GU_DEFINE_TYPE(PgfFunIndices, GuStringPtrMap, gu_type(PgfInferMap));

GU_SEQ_DEFINE(PgfFIdSeq, pgf_fid_seq, PgfFId);
static GU_DEFINE_TYPE(PgfFIdSeq, GuSeq, gu_type(PgfFId));

GU_INTPTRMAP_DEFINE(PgfCoerceIdx, pgf_coerce_idx, PgfFIdSeq);
static GU_DEFINE_TYPE(PgfCoerceIdx, GuIntPtrMap, gu_type(PgfFIdSeq));

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
	PgfLinInfers* entries = gu_map_get(infer_table, arg_fids);
	if (entries == NULL) {
		entries = pgf_lin_infers_new(lzr->pool);
		gu_map_set(infer_table, arg_fids, entries);
	} else {
		// XXX: arg_fids is duplicate, we ought to free it
		// Display warning?
	}

	PgfLinInferEntry entry = {
		.cat_id = fid,
		.fun = papply->fun
	};
	pgf_lin_infers_push(entries, entry);
}
			

static void
pgf_lzr_index(PgfLzr* lzr, PgfFId fid, PgfProduction prod)
{
	void* data = gu_variant_data(prod);
	switch (gu_variant_tag(prod)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		PgfInferMap* infer =
			gu_stringmap_get(lzr->fun_indices, papply->fun->fun);
		if (infer == NULL) {
			infer = gu_map_new(lzr->pool,
					   &pgf_lzr_fids_hash,
					   &pgf_lzr_fids_eq);
			gu_stringmap_set(lzr->fun_indices,
					 papply->fun->fun, infer);
		}
		pgf_lzr_add_infer_entry(lzr, infer, fid, papply);
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = data;
		PgfFIdSeq* fids = pgf_coerce_idx_get(lzr->coerce_idx,
						     pcoerce->coerce);
		if (fids == NULL) {
			fids = pgf_fid_seq_new(lzr->pool);
			pgf_coerce_idx_set(lzr->coerce_idx, 
					   pcoerce->coerce, fids);
		}
		pgf_fid_seq_push(fids, fid);
		break;
	}
	default:
		// Display warning?
		break;
	}
}

typedef struct {
	GuMapIterFn fn;
	PgfLzr* lzr;
} PgfLzrCreateIndexFn;


static void
pgf_lzr_create_index_cb(GuMapIterFn* fn, const void* key, void* value)
{
	PgfLzrCreateIndexFn* clo = (PgfLzrCreateIndexFn*) fn;
	const PgfFId* fid = key;
	PgfProductions* prods = value;

	for (int i = 0; i < prods->len; i++) {
		PgfProduction prod = prods->elems[i];
		pgf_lzr_index(clo->lzr, *fid, prod);
	}
}




PgfLzr*
pgf_lzr_new(GuPool* pool, PgfPGF* pgf, PgfConcr* cnc)
{
	PgfLzr* lzr = gu_new(pool, PgfLzr);
	lzr->pgf = pgf;
	lzr->cnc = cnc;
	lzr->pool = pool;
	lzr->fun_indices = gu_stringmap_new(pool);
	lzr->coerce_idx = pgf_coerce_idx_new(pool);
	PgfLzrCreateIndexFn clo = { { pgf_lzr_create_index_cb }, lzr };
	gu_map_iter((GuMap*)cnc->productions, &clo.fn);
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


static PgfFId 
pgf_lzn_pick_supercat(PgfLzn* lzn, PgfFId fid)
{
	while (true) {
		PgfFIdSeq* supers = pgf_coerce_idx_get(lzn->lzr->coerce_idx, fid);
		if (supers == NULL) {
			return fid;
		}
		int ch = gu_choice_next(lzn->ch, pgf_fid_seq_size(supers) + 1);
		if (ch == 0) {
			return fid;
		}
		fid = *pgf_fid_seq_index(supers, ch - 1);
	}
}

static PgfFId
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfLinForm* form_out);

static PgfFId
pgf_lzn_infer_apply_try(PgfLzn* lzn, PgfApplication* appl,
			PgfInferMap* infer, GuChoiceMark* marks,
			PgfFIds* arg_fids, int* ip, int n_args, 
			GuPool* pool, PgfLinFormApp* app_out)
{
	gu_enter("f: " GU_STRING_FMT ", *ip: %d, n_args: %d",
		 GU_STRING_FMT_ARGS(appl->fun), *ip, n_args);
	PgfFId ret = PGF_FID_INVALID;
	while (*ip < n_args) {
		PgfLinForm* arg_treep = 
			(app_out == NULL ? NULL : &app_out->args[*ip]);
		PgfFId arg_i = 
			pgf_lzn_infer(lzn, appl->args[*ip], pool, arg_treep);
		if (arg_i == PGF_FID_INVALID) {
			goto finish;
		}
		arg_i = pgf_lzn_pick_supercat(lzn, arg_i);
		gu_list_elems(arg_fids)[*ip] = arg_i;
		marks[++*ip] = gu_choice_mark(lzn->ch);
	}
	PgfLinInfers* entries = gu_map_get(infer, arg_fids);
	if (entries == NULL) {
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
	ret = entry->cat_id;
finish:
	gu_exit("fid: %d", ret);
	return ret;
}
typedef GuList(GuChoiceMark) PgfChoiceMarks;

static PgfFId
pgf_lzn_infer_application(PgfLzn* lzn, PgfApplication* appl, 
			  GuPool* pool, PgfLinForm* form_out)
{
	PgfInferMap* infer = gu_stringmap_get(lzn->lzr->fun_indices, appl->fun);
	gu_enter("f: " GU_STRING_FMT ", n_args: %d",
		 GU_STRING_FMT_ARGS(appl->fun), appl->n_args);
	if (infer == NULL) {
		return PGF_FID_INVALID;
	}
	GuPool* tmp_pool = gu_pool_new();
	PgfFId ret = PGF_FID_INVALID;
	int n = appl->n_args;
	PgfFIds* arg_fids = gu_list_new(PgfFIds, tmp_pool, n);

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
					      marks, arg_fids,
					      &i, n, pool, appt);
		if (ret != PGF_FID_INVALID) {
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
	gu_exit("fid: %d", ret);
	return ret;
}

static PgfFId
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfLinForm* form_out)
{
	PgfFId ret = PGF_FID_INVALID;
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
			ret = pgf_literal_fid(elit->lit);
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
	PgfFId c_id = PGF_FID_INVALID;
	GuChoiceMark mark = gu_choice_mark(lzn->ch);
	do {
		c_id = pgf_lzn_infer(lzn, lzn->expr, NULL, NULL);
		gu_choice_reset(lzn->ch, mark);
	} while (c_id == PGF_FID_INVALID && gu_choice_advance(lzn->ch));

	if (c_id != PGF_FID_INVALID) {
		PgfFId c_id2 = pgf_lzn_infer(lzn, lzn->expr, pool, &form);
		gu_assert(c_id == c_id2);
		gu_debug("c_id: %d", c_id);
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
		PgfSequence* seq = *(fun->lins[lin_idx]);
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
			case PGF_SYMBOL_KP:
				// XXX: To be supported
				gu_impossible(); 
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
		PgfToken* tok = toks->elems[i];
		fprintf(flin->file, 
			GU_STRING_FMT " ", GU_STRING_FMT_ARGS(tok));
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
