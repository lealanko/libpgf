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


struct PgfLinearizer {
	PgfPGF* pgf;
	PgfConcr* cnc;
	GuPool* pool;
	GuMap* prods; // PgfCId |-> GuIntMap |-> GPtrArray |-> PgfProduction
	/**< Productions per function. This map associates with each
	 * abstract function (as identified by a CId) the productions
	 * that apply that function's linearization. */
};

typedef PgfLinearizer PgfLzr;


static void
pgf_lzr_parr_free_cb(void* p)
{
	GPtrArray* parr = p;
	g_ptr_array_free(parr, TRUE);
}

static void
pgf_linearizer_add_production(PgfLinearizer* lzr, PgfFId fid,
			      PgfProduction prod, PgfProduction from)
{
	void* data = gu_variant_data(from);
	switch (gu_variant_tag(from)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		GuMap* m = gu_map_get(lzr->prods, papply->fun[0]->fun);
		if (m == NULL) {
			m = gu_intmap_new(lzr->pool);
			gu_map_set(lzr->prods, papply->fun[0]->fun, m);
		}
		GPtrArray* lin_prods = gu_intmap_get(m, fid);
		if (lin_prods == NULL) {
			lin_prods = g_ptr_array_new();
			gu_pool_finally(lzr->pool, 
					pgf_lzr_parr_free_cb, 
					lin_prods);
			gu_intmap_set(m, fid, lin_prods);
		}
		g_ptr_array_add(lin_prods, gu_variant_to_ptr(prod));
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
			pgf_linearizer_add_production(lzr, fid, prod,
						      c_prods->elems[i]);
		}
		return;
	}
	default:
		return;
	}
}

static void
pgf_linearizer_add_production_cb(void* key, void* value, 
				 void* user_data)
{
	PgfFId fid = GPOINTER_TO_INT(key);
	PgfProductions* prods = value;
	PgfLinearizer* lzr = user_data;

	for (int i = 0; i < prods->len; i++) {
		PgfProduction prod = prods->elems[i];
		pgf_linearizer_add_production(lzr, fid, prod, prod);
	}
}

PgfLinearizer*
pgf_linearizer_new(GuPool* pool, PgfPGF* pgf, PgfConcr* cnc)
{
	PgfLinearizer* lzr = gu_new(pool, PgfLinearizer);
	lzr->pgf = pgf;
	lzr->cnc = cnc;
	lzr->pool = pool;
	lzr->prods = gu_map_new(pool, gu_string_hash, gu_string_equal);
	g_hash_table_foreach(cnc->productions, pgf_linearizer_add_production_cb,
			     lzr);
	// TODO: prune productions with zero linearizations
	return lzr;
}

struct PgfLinearization {
	PgfLinearizer* lzr;
	GByteArray* path;
};

typedef PgfLinearization PgfLzn;

PgfLzn*
pgf_lzn_new(GuPool* pool, PgfLzr* lzr)
{
	PgfLzn* lzn = gu_new(pool, PgfLzn);
	lzn->lzr = lzr;
	lzn->path = g_byte_array_new();
	gu_pool_finally(pool, 
			// Not portable
			(GDestroyNotify)g_byte_array_unref,
			lzn->path);
	return lzn;
}




static bool
pgf_lzn_advance(PgfLzn* lzn)
{
	GByteArray* path = lzn->path;
	while (path->len > 0 && path->data[path->len - 1] == 1) {
		g_byte_array_set_size(path, path->len - 1);
	}
	if (path->len == 0) {
		return FALSE;
	} else {
		g_assert(path->data[path->len - 1] > 1);
		path->data[path->len - 1]--;
		return TRUE;
	}
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
	      int lin_idx, PgfCId* fun_cid, GPtrArray* args);

bool
pgf_lzn_linearize(PgfLzn* lzn, PgfExpr expr, PgfFId fid, int lin_idx, 
		  PgfLinFuncs* cb_fns, void* cb_ctx)
{
	PgfLzc lzc = {
		.lzn = lzn,
		.expr = expr,
		.path_idx = 0,
		.cb_fns = cb_fns,
		.cb_ctx = cb_ctx,
	};
	return pgf_lzc_linearize(&lzc, expr, fid, lin_idx);
}



// TODO: check for invalid lin_idx. For literals, only 0 is legal.
static bool
pgf_lzc_linearize(PgfLzc* lzc, PgfExpr expr, PgfFId fid, int lin_idx) 
{
	GPtrArray* args = g_ptr_array_new();
	bool succ = FALSE;

	while (TRUE) {
		GuVariantInfo i = gu_variant_open(expr);
		switch (i.tag) {
		case PGF_EXPR_APP: {
			PgfExprApp* app = i.data;
			g_ptr_array_add(args, gu_variant_to_ptr(app->arg));
			expr = app->fun;
			break;
		}
		case PGF_EXPR_IMPL_ARG: {
			PgfExprImplArg* eimplarg = i.data;
			expr = *eimplarg;
			break;
		}
		case PGF_EXPR_TYPED: {
			PgfExprTyped* etyped = i.data;
			expr = etyped->expr;
			break;
		}
		case PGF_EXPR_FUN: {
			PgfExprFun* fun = i.data;
			PgfCId* fun_cid = *fun;
			succ = pgf_lzc_apply(lzc, expr, fid, lin_idx, 
					     fun_cid, args);
			goto exit;
			
	 	}
		case PGF_EXPR_LIT: {
			PgfExprLit* elit = i.data;
			PgfLiteral lit = *elit;
			g_assert(args->len == 0); // XXX: fail nicely
			lzc->cb_fns->expr_literal(lzc->cb_ctx, lit);
			succ = TRUE;
			goto exit;
		}
		case PGF_EXPR_ABS:
		case PGF_EXPR_VAR:
		case PGF_EXPR_META:
			g_assert_not_reached(); // XXX
			break;
			
		default:
			g_assert_not_reached();
		}
	}
exit:
	g_ptr_array_free(args, FALSE);
	return succ;
}

static PgfProductionApply*
pgf_lzc_choose_production(PgfLzc* lzc, PgfCId* cid, PgfFId fid)
{
	PgfLzn* lzn = lzc->lzn;
	PgfLzr* lzr = lzn->lzr;

	GuIntMap* prod_map = gu_map_get(lzr->prods, cid);
	if (prod_map == NULL) {
		return NULL;
	}
	
	while (TRUE) {
		GPtrArray* prods = gu_intmap_get(prod_map, fid);
		if (prods == NULL || prods->len == 0) {
			return NULL;
		}

		int idx = 0;
		if (prods->len > 1) {
			if (lzc->path_idx == (int) lzn->path->len) {
				uint8_t n = (uint8_t)prods->len;
				g_byte_array_append(lzn->path, &n, 1);
			}
			g_assert(lzc->path_idx < (int) lzn->path->len);
			uint8_t n = lzn->path->data[lzc->path_idx];
			idx = prods->len - n;
			lzc->path_idx++;
		}
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
		}
		default:
			g_assert_not_reached();
		}
	}
}

static bool
pgf_lzc_apply(PgfLzc* lzc, PgfExpr src_expr, PgfFId fid,
	      int lin_idx, PgfCId* fun_cid, GPtrArray* args)
{
	PgfLzn* lzn = lzc->lzn;
	PgfLzr* lzr = lzn->lzr;
	PgfPGF* pgf = lzr->pgf;
	PgfLinFuncs* fns = lzc->cb_fns;
	int nargs = (int) args->len;

	PgfFunDecl* fdecl = gu_map_get(pgf->abstract.funs, fun_cid);
	g_assert(fdecl != NULL); // XXX: turn this into a proper check
	g_assert(nargs == fdecl->arity); // XXX: ditto
	
	PgfProductionApply* papply = 
		pgf_lzc_choose_production(lzc, fun_cid, fid);
	if (papply == NULL) {
		return FALSE;
	}

	g_assert(papply->n_args == nargs);
	
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
			g_assert(sidx->d < nargs);
			PgfExpr arg = 
				gu_variant_from_ptr(args->pdata[sidx->d]);
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
				fns->symbol_tokens(lzc->cb_ctx, ks);
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


static void
pgf_file_lzn_symbol_tokens(void* ctx, PgfTokens* toks)
{
	FILE* out = ctx;
	
	for (int i = 0; i < toks->len; i++) {
		PgfToken* tok = toks->elems[i];
		fprintf(out, "%.*s ", tok->len, tok->elems);
	}
}

static PgfLinFuncs pgf_file_lin_funcs = {
	.symbol_tokens = pgf_file_lzn_symbol_tokens
};

bool
pgf_lzn_linearize_to_file(PgfLzn* lzn, PgfExpr expr, PgfFId fid, int lin_idx, 
			  FILE* file_out)
{
	return pgf_lzn_linearize(lzn, expr, fid, lin_idx, 
				 &pgf_file_lin_funcs, file_out);
}
