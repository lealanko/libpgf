// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

#include <libgu.h>
#include <pgf/pgf.h>
#include <pgf/expr.h>

/// Linearization of abstract syntax trees.
/// @file

/** @name Linearizers
 *
 * Linearization begins by choosing a concrete category (#PgfConcr) for some
 * grammar, and creating a new linearizer (#PgfLzr) which can then be used to
 * linearize abstract syntax trees (#PgfExpr) of that grammar into the given
 * concrete category.
 *
 * @{
 */


/// A linearizer.
typedef struct PgfLzr PgfLzr;
/**<
 *
 * A #PgfLzr object transforms abstract syntax trees of a PGF grammar
 * into sequences of token events for a single concrete category of
 * that grammar.
 * 
 */
GU_DECLARE_TYPE(PgfLzr, struct);


/// Create a new linearizer.
PgfLzr*
pgf_new_lzr(PgfConcr* cnc, GuPool* pool);
/**<
 * @param cnc The concrete category to linearize to. 
 *
 * @pool
 *
 * @return A new linearizer.
 */

/** @}
 *
 * @name Enumerating concrete syntax trees
 *
 * Because of the `variants` construct in GF, there may be several
 * possible concrete syntax trees that correspond to a given abstract
 * syntax tree. These can be enumerated with #pgf_lzr_concretize and
 * #gu_enum_next.
 *
 * @{
 */


/// A concrete syntax tree
typedef GuVariant PgfCncTree;

/// An enumeration of #PgfCncTree trees.
typedef GuEnum PgfCncTreeEnum;

/// Begin enumerating concrete syntax variants.
PgfCncTreeEnum*
pgf_lzr_concretize(PgfLzr* lzr, PgfExpr expr, GuPool* pool);

/** @}
 *
 * @name Linearizing concrete syntax trees
 *
 * An individual concrete syntax tree has several different constituents,
 * corresponding to the various fields and cases of corresponding GF values.
 * The number of these constituents can be retrieved with
 * #pgf_cnc_tree_n_ctnts.
 *  
 * A single linearization of a concrete syntax tree is performed by
 * #pgf_lzr_linearize. The linearization is realized as a sequence of
 * events that are notified by calling the functions of a #PgfLinFuncs
 * structure that the client provides.
 *
 * @{
 */

typedef struct PgfPresenter PgfPresenter;
/// Callback functions for linearization.
typedef struct PgfPresenterFuns PgfPresenterFuns;

struct PgfPresenter {
	PgfPresenterFuns* funs;
};

struct PgfPresenterFuns
{
	/// Output tokens
	void (*symbol_tokens)(PgfPresenter* self, PgfTokens toks);

	void (*symbol_expr)(PgfPresenter* self, 
			    int argno, PgfExpr expr, PgfCtntId ctnt);

	/// Begin application
	void (*expr_apply)(PgfPresenter* self, PgfCId cid, int n_args);

	/// Output literal
	void (*expr_literal)(PgfPresenter* self, PgfLiteral lit);

	void (*abort)(PgfPresenter* self);
	void (*finish)(PgfPresenter* self);
};





/// Linearize a concrete syntax tree.
void
pgf_lzr_linearize(PgfLzr* lzr, PgfCncTree ctree, PgfCtntId ctnt,
		  PgfPresenter* fnsp);


/// Linearize a concrete syntax tree as space-separated tokens.
void
pgf_lzr_linearize_simple(PgfLzr* lzr, PgfCncTree ctree,
			 PgfCtntId ctnt, GuWriter* wtr, GuExn* err);


/// Return the dimension of a concrete syntax tree.
int
pgf_cnc_tree_n_ctnts(PgfCncTree ctree);
/**<
 * @param ctree A concrete syntax tree.
 *
 * @return The dimension of the tree, i.e. the number of different
 * linearizations the tree has.
 */

//@}



extern GuTypeTable
pgf_linearize_dump_table;

