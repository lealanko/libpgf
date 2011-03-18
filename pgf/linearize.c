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
#include <pgf/expr.h>
// Type and dump for GPtrArray

typedef const struct PgfGPtrArrayType PgfGPtrArrayType, GuType_GPtrArray;

struct PgfGPtrArrayType {
	GuType_abstract abstract_base;
	GuType* elem_type;
};

typedef const struct PgfGArrayType PgfGArrayType, GuType_GArray;

struct PgfGArrayType {
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

#define GU_TYPE_INIT_GArray(k_, t_, elem_type_) {	   \
	.abstract_base = GU_TYPE_INIT_abstract(k_, t_, _), \
	.elem_type = elem_type_,		   	\
}

GU_DEFINE_KIND(GArray, abstract);

static void
pgf_dump_garray(GuDumpFn* dumper, GuType* type, const void* p, 
		   GuDumpCtx* ctx)
{
	(void) dumper;
	PgfGArrayType* atype = gu_type_cast(type, GArray);
	const GArray* arr = p;
	size_t size = gu_type_size(atype->elem_type);
	gu_yaml_begin_sequence(ctx->yaml);
	for (int i = 0; i < (int) arr->len; i++) {
		gu_dump(atype->elem_type,
			&arr->data[i * size], ctx);
	}
	gu_yaml_end(ctx->yaml);
}

GuTypeTable
pgf_linearize_dump_table = GU_TYPETABLE(
	GU_SLIST_0,
	{ gu_kind(GPtrArray), gu_fn(pgf_dump_gptrarray) },
	{ gu_kind(GArray), gu_fn(pgf_dump_garray) },
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
	GuMap* infer; // PgfFIds |-> GPtrArray [PgfFId]
};

/*
GU_DEFINE_TYPE(
	PgfLinIndex, struct,
	GU_MEMBER(PgfLinIndex, prods,
		  GU_TYPE_LIT(GuIntMap, GuIntMap,
		  GU_TYPE_LIT(pointer, GPtrArray*,
		  GU_TYPE_LIT(GPtrArray, GPtrArray, 
		  GU_TYPE_LIT(GuVariantAsPtr,
			      gu_type(PgfProduction)))))),
	GU_MEMBER(PgfLinIndex, infer,
		  GU_TYPE_LIT(GuMap, GuMap,
			      gu_type(PgfFIds),
			      gu_type
*/					  

struct PgfLzr {
	PgfPGF* pgf;
	PgfConcr* cnc;
	GuPool* pool;
	GuStringMap* fun_indices; // PgfCId |-> PgfLinIndex
};

GU_DEFINE_TYPE(
	PgfLzr, struct,
	GU_MEMBER_P(PgfLzr, pgf, PgfPGF),
	GU_MEMBER_P(PgfLzr, cnc, PgfConcr),
//	GU_MEMBER_P(PgfLzr, prods, PgfLinProds)
	);
 

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
pgf_lzr_add_infer_entry(PgfLzr* lzr,
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
		entries = pgf_lzr_arr_new(lzr->pool,
					     sizeof(PgfLinInferEntry));
		gu_map_set(infer_table, arg_fids, entries);
	} else {
		// XXX: arg_fids is duplicate, we ought to free it
	}

	PgfLinInferEntry entry = {
		.cat_id = fid,
		// XXX: just store the raw id in productions
		.fun_id = papply->fun - gu_list_elems(lzr->cnc->cncfuns),
	};
	g_array_append_val(entries, entry);
}
			

static void
pgf_lzr_add_linprods_entry(PgfLzr* lzr,
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
pgf_lzr_index(PgfLzr* lzr, PgfFId fid,
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
	PgfLzr* lzr = user_data;

	for (int i = 0; i < prods->len; i++) {
		PgfProduction prod = prods->elems[i];
		pgf_lzr_index(lzr, fid, prod, prod);
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
	g_hash_table_foreach(cnc->productions, pgf_lzr_create_index_cb, lzr);
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
	GArray* entries = gu_map_get(idx->infer, arg_fids);
	if (entries == NULL) {
		--*ip;
		goto finish;
	}
	//do {
	int e = gu_choice_next(lzn->ch, entries->len);
	gu_debug("entry %d of %d", e, entries->len);
		if (e >= 0) {
			PgfLinInferEntry* entry = 
			&g_array_index(entries, PgfLinInferEntry, e);
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
	PgfLinIndex* idx = gu_map_get(lzn->lzr->fun_indices, appl->fun);
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

static void 
pgf_lzn_byte_array_free_cb(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	g_byte_array_unref(clo->env1);
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
				g_assert_not_reached(); 
			default:
				g_assert_not_reached(); 
			}
		}
		break;
	} // case PGF_LIN_FORM_APP
	default:
		g_assert_not_reached();
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
