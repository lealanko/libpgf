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
#include <gu/map.h>


typedef struct PgfLinearizer PgfLinearizer;

struct PgfLinearizer {
	PgfPGF* pgf;
	PgfConcr* cnc;
	GuMemPool* pool;
	GuMap* prods; // PgfCId |-> GuIntMap |-> GPtrArray |-> PgfProduction
	/**< Productions per function. This map associates with each
	 * abstract function (as identified by a CId) the productions
	 * that apply that function's linearization. */
};

typedef PgfLinearizer PgfLzr;



static void
pgf_linearizer_add_production(PgfLinearizer* lzr, PgfFId fid,
			      PgfProduction prod, PgfProduction from)
{
	gpointer data = gu_variant_data(from);
	switch (gu_variant_tag(from)) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papply = data;
		GuMap* m = gu_map_get(lzr->prods, *papply->fun);
		if (m == NULL) {
			m = gu_intmap_new(lzr->pool);
			gu_map_set(lzr->prods, *papply->fun, m);
		}
		GPtrArray* lin_prods = gu_intmap_get(m, fid);
		if (lin_prods == NULL) {
			lin_prods = g_ptr_array_new();
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
		for (gint i = 0; i < c_prods->len; i++) {
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
pgf_linearizer_add_production_cb(gpointer key, gpointer value, 
				 gpointer user_data)
{
	PgfFId fid = GPOINTER_TO_INT(key);
	PgfProductions* prods = value;
	PgfLinearizer* lzr = user_data;

	for (gint i = 0; i < prods->len; i++) {
		PgfProduction prod = prods->elems[i];
		pgf_linearizer_add_production(lzr, fid, prod, prod);
	}
}


PgfLinearizer*
pgf_linearizer_new(PgfPGF* pgf, PgfConcr* cnc)
{
	PgfLinearizer* lzr = g_slice_new(PgfLinearizer);
	lzr->pgf = pgf;
	lzr->cnc = cnc;
	lzr->prods = g_hash_table_new(gu_string_hash, gu_string_equal);
	g_hash_table_foreach(cnc->productions, pgf_linearizer_add_production_cb,
			     lzr);
	// TODO: prune productions with zero linearizations
}

typedef struct PgfLinearization PgfLinearization;

struct PgfLinearization {
	PgfLinearizer* lzr;
	GByteArray* path;
};

typedef PgfLinearization PgfLzn;

typedef GuVariant PgfCncTree;

typedef GuList(PgfCncTree) PgfCncTrees;

typedef enum {
	PGF_CNCTREE_LEAFKS,
	PGF_CNCTREE_LEAFKP,
	PGF_CNCTREE_LEAFLIT,
	PGF_CNCTREE_BRACKET
} PgfCncTreeTag;

typedef PgfTokens PgfCncTreeLeafKS;

typedef struct PgfCncTreeLeafLit PgfCncTreeLeafLit;
struct PgfCncTreeLeafLit {
	PgfLiteral lit;
};

// XXX: PgfCncTermLeafKP, although fully resolved prefix tokens would
// be better

typedef struct PgfCncTreeBracket PgfCncTreeBracket;

struct PgfCncTreeBracket {
	// XXX: metainformation
	PgfExpr src_expr;
	PgfCId* cat_cid;
	gint lin_idx;
	gint n_subtrees;
	PgfCncTree subtrees[];
};

static PgfLzn*
pgf_lzn_new(PgfLzr* lzr, GuMemPool* pool)
{
	GuAllocator* ator = gu_mem_pool_allocator(pool);
	PgfLzn* lzn = gu_new(ator, PgfLzn);
	lzn->lzr = lzr;
	lzn->path = g_byte_array_new();
	gu_mem_pool_register_finalizer(pool, 
				       // Not portable
				       (GDestroyNotify)g_byte_array_unref,
				       lzn->path);
	return lzn;
}




static gboolean
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
	GuMemPool* pool;
	GuAllocator* ator;
	gint path_idx;
};

static PgfCncTree
pgf_lzc_linearize(PgfLzc* lzc, PgfExpr expr, PgfFId fid, gint lin_idx);

static PgfCncTree 
pgf_lzc_apply(PgfLzc* lzc, PgfExpr src_expr, PgfFId fid,
	      gint lin_idx, PgfCId* fun_cid, GPtrArray* args);

PgfCncTree
pgf_lzn_linearize(PgfLzn* lzn, PgfExpr expr, PgfFId fid, gint lin_idx, 
		  GuMemPool* pool)
{
	PgfLzc lzc = {
		.lzn = lzn,
		.expr = expr,
		.pool = pool,
		.ator = gu_mem_pool_allocator(pool),
		.path_idx = 0
	};
	return pgf_lzc_linearize(&lzc, expr, fid, lin_idx);
}


static PgfCncTree
pgf_lzc_lit(PgfLzc* lzc, PgfLiteral lit)
{
	PgfCncTree tree = gu_variant_null;
	PgfCncTreeLeafLit* tlit =
		gu_variant_new(lzc->ator, PGF_CNCTREE_LEAFLIT, 
			       PgfCncTreeLeafLit, &tree);
	tlit->lit = lit;
	return tree;
}


// TODO: check for invalid lin_idx. For literals, only 0 is legal.
static PgfCncTree
pgf_lzc_linearize(PgfLzc* lzc, PgfExpr expr, PgfFId fid, gint lin_idx) 
{
	GPtrArray* args = g_ptr_array_new();
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
			return pgf_lzc_apply(lzc, expr, fid, lin_idx, 
					     fun_cid, args);
		}
		case PGF_EXPR_LIT: {
			PgfExprLit* elit = i.data;
			PgfLiteral lit = *elit;
			g_assert(args->len == 0); // XXX: fail nicely
			return pgf_lzc_lit(lzc, lit);
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

		gint idx = 0;
		if (prods->len > 1) {
			if (lzc->path_idx == lzn->path->len) {
				guint8 n = (guint8)prods->len;
				g_byte_array_append(lzn->path, &n, 1);
			}
			g_assert(lzc->path_idx < lzn->path->len);
			guint8 n = lzn->path->data[lzc->path_idx];
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

static PgfCncTree 
pgf_lzc_apply(PgfLzc* lzc, PgfExpr src_expr, PgfFId fid,
	      gint lin_idx, PgfCId* fun_cid, GPtrArray* args)
{
	PgfLzn* lzn = lzc->lzn;
	PgfLzr* lzr = lzn->lzr;
	PgfPGF* pgf = lzr->pgf;

	PgfFunDecl* fdecl = gu_map_get(pgf->abstract.funs, fun_cid);
	g_assert(fdecl != NULL); // XXX: turn this into a proper check
	g_assert(args->len == fdecl->arity); // XXX: ditto
	
	PgfProductionApply* papply = 
		pgf_lzc_choose_production(lzc, fun_cid, fid);
	if (papply == NULL) {
		return gu_variant_null;
	}

	g_assert(papply->n_args == args->len);
	
	PgfCncFun* cfun = *papply->fun;
	g_assert(lin_idx < cfun->n_lins);
	
	PgfSequence* lin = *(cfun->lins[lin_idx]);
	
	PgfCncTree tree = gu_variant_null;

	PgfCncTreeBracket* bracket = 
		gu_variant_flex_new(lzc->ator, 
				    PGF_CNCTREE_BRACKET,
				    PgfCncTreeBracket,
				    subtrees,
				    lin->len,
				    &tree);
	bracket->n_subtrees = lin->len;
	bracket->lin_idx = lin_idx;
	bracket->cat_cid = fdecl->type->cid;
	bracket->src_expr = src_expr;
	
	for (gint i = 0; i < bracket->n_subtrees; i++) {
		PgfSymbol sym = lin->elems[i];
		GuVariantInfo sym_i = gu_variant_open(sym);
		switch (sym_i.tag) {
		case PGF_SYMBOL_CAT:
		case PGF_SYMBOL_VAR:
		case PGF_SYMBOL_LIT: {
			PgfSymbolIdx* sidx = sym_i.data;
			g_assert(sidx->d < args->len);
			PgfExpr arg = 
				gu_variant_from_ptr(args->pdata[sidx->d]);
			bracket->subtrees[i] =
				pgf_lzc_linearize(lzc, 
						  arg,
						  papply->args[sidx->d]->fid,
						  sidx->r);
			if (gu_variant_is_null(bracket->subtrees[i])) {
				return gu_variant_null;
			}
			break;
		}
		case PGF_SYMBOL_KS: {
			PgfSymbolKS* ks = sym_i.data;
			PgfCncTreeLeafKS* leafks =
				gu_variant_flex_new(lzc->ator,
						    PGF_CNCTREE_LEAFKS,
						    PgfCncTreeLeafKS,
						    elems,
						    ks->len,
						    &bracket->subtrees[i]);
			break;
		}
		case PGF_SYMBOL_KP:
			// XXX: To be supported
			g_assert_not_reached(); 
		default:
			g_assert_not_reached(); 
		}
	}
	
	return tree;
}

