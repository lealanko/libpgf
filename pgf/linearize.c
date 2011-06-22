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
	PgfFId fun_id;
};

static GU_DEFINE_TYPE(
	PgfLinInferEntry, struct,
	GU_MEMBER(PgfLinInferEntry, cat_id, PgfFId),
	GU_MEMBER(PgfLinInferEntry, fun_id, PgfFId));

GU_SEQ_DEFINE(PgfLinInfers, pgf_lin_infers, PgfLinInferEntry);
static GU_DEFINE_TYPE(PgfLinInferSeq, GuSeq, gu_type(PgfLinInferEntry));

typedef GuIntMap PgfCncProds;
static GU_DEFINE_TYPE(PgfCncProds, GuIntMap, gu_ptr_type(PgfProdSeq));

typedef GuStringMap PgfLinProds;
static GU_DEFINE_TYPE(PgfLinProds, GuStringMap, gu_ptr_type(PgfCncProds));

typedef GuMap PgfInferMap;
static GU_DEFINE_TYPE(PgfInferMap, GuMap, gu_ptr_type(PgfFIds), gu_ptr_type(PgfLinInferEntry));

typedef struct PgfLinIndex PgfLinIndex;

struct PgfLinIndex {
	PgfCncProds* prods; 
	PgfInferMap* infer;
};

GU_DEFINE_TYPE(
	PgfLinIndex, struct,
	GU_MEMBER_P(PgfLinIndex, prods, PgfCncProds),
	GU_MEMBER_P(PgfLinIndex, infer, PgfInferMap));


typedef GuStringMap PgfFunIndices;
static GU_DEFINE_TYPE(PgfFunIndices, GuStringMap, gu_ptr_type(PgfLinIndex));

struct PgfLzr {
	PgfPGF* pgf;
	PgfConcr* cnc;
	GuPool* pool;
	PgfFunIndices* fun_indices;
};

GU_DEFINE_TYPE(
	PgfLzr, struct,
	GU_MEMBER_P(PgfLzr, pgf, PgfPGF),
	GU_MEMBER_P(PgfLzr, cnc, PgfConcr),
	GU_MEMBER_P(PgfLzr, fun_indices, PgfFunIndices));

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
	}

	PgfLinInferEntry entry = {
		.cat_id = fid,
		// XXX: just store the raw id in productions
		.fun_id = papply->fun - gu_list_elems(lzr->cnc->cncfuns),
	};
	pgf_lin_infers_push(entries, entry);
}
			

static void
pgf_lzr_add_linprods_entry(PgfLzr* lzr,
			   PgfCncProds* linprods,
			   PgfFId fid,
			   PgfProduction prod)
{
	PgfProdSeq* prods = gu_intmap_get(linprods, fid); 
	if (prods == NULL) {
		prods = pgf_prod_seq_new(lzr->pool);
		gu_intmap_set(linprods, fid, prods);
	}
	pgf_prod_seq_push(prods, prod);
}


static void
pgf_lzr_index(PgfLzr* lzr, PgfFId fid,
	      PgfProduction prod, PgfProduction from)
{
	void* data = gu_variant_data(from);
	switch (gu_variant_tag(from)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		PgfLinIndex* idx =
			gu_stringmap_get(lzr->fun_indices, papply->fun[0]->fun);
		if (idx == NULL) {
			idx = gu_new(lzr->pool, PgfLinIndex);
			idx->prods = gu_intmap_new(lzr->pool);
			idx->infer = gu_map_new(lzr->pool,
						&pgf_lzr_fids_hash,
						&pgf_lzr_fids_eq);
			gu_stringmap_set(lzr->fun_indices,
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
		pgf_lzr_index(clo->lzr, *fid, prod, prod);
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
	PgfLzrCreateIndexFn clo = { { pgf_lzr_create_index_cb }, lzr };
	gu_map_iter(cnc->productions, &clo.fn);
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
	PgfFId fun_id;
	GuLength n_args;
	PgfLinForm args[];
};

typedef struct PgfLinFormLit PgfLinFormLit;
struct PgfLinFormLit {
	PgfLiteral lit;
};


static PgfFId
pgf_lzn_infer(PgfLzn* lzn, PgfExpr expr, GuPool* pool, PgfLinForm* form_out);

static PgfFId
pgf_lzn_infer_apply_try(PgfLzn* lzn, PgfApplication* appl,
			PgfLinIndex* idx, GuChoiceMark* marks,
			PgfFIds* arg_fids, int* ip, int n_args, 
			GuPool* pool, PgfLinFormApp* app_out)
{
	gu_enter("f: " GU_STRING_FMT ", *ip: %d, n_args: %d",
		 GU_STRING_FMT_ARGS(appl->fun), *ip, n_args);
	PgfFId ret = PGF_FID_INVALID;
	while (*ip < n_args) {
		marks[*ip] = gu_choice_mark(lzn->ch);
		PgfLinForm* arg_treep = 
			(app_out == NULL ? NULL : &app_out->args[*ip]);
		PgfFId arg_i = 
			pgf_lzn_infer(lzn, appl->args[*ip], pool, arg_treep);
		if (arg_i == PGF_FID_INVALID) {
			goto finish;
		}
		gu_list_elems(arg_fids)[*ip] = arg_i;
		++*ip;
	}
	marks[*ip] = gu_choice_mark(lzn->ch);
	PgfLinInfers* entries = gu_map_get(idx->infer, arg_fids);
	if (entries == NULL) {
		--*ip;
		goto finish;
	}
	int n_entries = pgf_lin_infers_size(entries);
	//do {
	int e = gu_choice_next(lzn->ch, n_entries);
	gu_debug("entry %d of %d", e, n_entries);
		if (e >= 0) {
			PgfLinInferEntry* entry = 
				pgf_lin_infers_index(entries, e);
			if (app_out != NULL) {
				app_out->fun_id = entry->fun_id;
			}
			ret = entry->cat_id;
			goto finish;
		}
		//gu_choice_reset(lzn->ch, marks[*ip]);
	//} while (gu_choice_advance(lzn->ch));
	--*ip;
finish:
	gu_exit("fid: %d", ret);
	return ret;
}
typedef GuList(GuChoiceMark) PgfChoiceMarks;

static PgfFId
pgf_lzn_infer_application(PgfLzn* lzn, PgfApplication* appl, 
			  GuPool* pool, PgfLinForm* form_out)
{
	PgfLinIndex* idx = gu_stringmap_get(lzn->lzr->fun_indices, appl->fun);
	gu_enter("f: " GU_STRING_FMT ", n_args: %d",
		 GU_STRING_FMT_ARGS(appl->fun), appl->n_args);
	if (idx == NULL) {
		return PGF_FID_INVALID;
	}
	GuPool* tmp_pool = gu_pool_new();
	PgfFId ret = PGF_FID_INVALID;
	int n = appl->n_args;
	PgfFIds* arg_fids = gu_list_new(PgfFIds, tmp_pool, n);

	// GuChoiceMark marks[n];
	PgfChoiceMarks* marksl = gu_list_new(PgfChoiceMarks, tmp_pool, n + 1);
	GuChoiceMark* marks = gu_list_elems(marksl);
	
	PgfLinFormApp* appt = NULL;
	if (form_out) {
		appt = gu_variant_flex_new(pool, PGF_LIN_FORM_APP, PgfLinFormApp, 
					   args, n, form_out);
		appt->n_args = n;
	}

	int i = 0;
	while (true) {
		ret = pgf_lzn_infer_apply_try(lzn, appl, idx,
					      marks, arg_fids,
					      &i, n, pool, appt);
		if (ret != PGF_FID_INVALID) {
			break;
		}
		break; // XXX
		while (true) {
			gu_choice_reset(lzn->ch, marks[i]);
			if (!gu_choice_advance(lzn->ch)) {
				if (i == 0) {
					goto finish;
				} else {
					i--;
				}
			} else {
				break;
			}
		}
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
		gu_choice_reset(lzn->ch, mark);
		if (!gu_choice_advance(lzn->ch)) {
			lzn->expr = gu_variant_null;
		}
	}
	return form;
}




void
pgf_lzr_linearize(PgfLzr* lzr, PgfLinForm form, int lin_idx, PgfLinFuncs** fnsp)
{
	PgfLinFuncs* fns = *fnsp;
	GuVariantInfo formi = gu_variant_open(form);

	switch (formi.tag) {
	case PGF_LIN_FORM_LIT: {
		PgfLinFormLit* flit = formi.data;
		if (fns->expr_literal) {
			fns->expr_literal(fnsp, flit->lit);
		}
		break;
	}
	case PGF_LIN_FORM_APP: {
		PgfLinFormApp* fapp = formi.data;
		// XXX: validate instead of assert
		gu_assert(fapp->fun_id < gu_list_length(lzr->cnc->cncfuns));
		PgfCncFun* fun =
			gu_list_elems(lzr->cnc->cncfuns)[fapp->fun_id];
		if (fns->expr_apply) {
			fns->expr_apply(fnsp, fun->fun, fapp->n_args);
		}
		// XXX: validate instead of assert
		gu_assert(lin_idx < fun->n_lins); 
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
