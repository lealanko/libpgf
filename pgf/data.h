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

#ifndef PGF_DATA_H_
#define PGF_DATA_H_

#include <gu/list.h>
#include <gu/variant.h>
#include <gu/map.h>
#include <gu/type.h>
#include <gu/seq.h>
#include <gu/string.h>
#include <pgf/pgf.h>
#include <pgf/expr.h>

typedef struct PgfCCat PgfCCat;
typedef PgfCCat* PgfCCatId;
extern GU_DECLARE_TYPE(PgfCCat, struct);
extern GU_DECLARE_TYPE(PgfCCatId, shared);
typedef GuSeq PgfCCatIds;
extern GU_DECLARE_TYPE(PgfCCatIds, GuSeq);
typedef GuSeq PgfCCats;
extern GU_DECLARE_TYPE(PgfCCats, GuSeq);

typedef struct PgfAbstr PgfAbstr;
typedef struct PgfFunDecl PgfFunDecl;
typedef struct PgfConcr PgfConcr;

typedef int PgfLength;
typedef struct GuVariant PgfSymbol;
typedef struct PgfAlternative PgfAlternative;
typedef GuSeq PgfAlternatives;
typedef struct PgfCncFun PgfCncFun;


typedef GuSeq PgfSequence; // -> PgfSymbol

typedef PgfCncFun* PgfFunId; // key to PgfCncFuns
extern GU_DECLARE_TYPE(PgfFunId, shared);
typedef GuSeq PgfCncFuns; 
extern GU_DECLARE_TYPE(PgfCncFuns, GuSeq);
typedef GuSeq PgfFunIds; 
extern GU_DECLARE_TYPE(PgfFunIds, GuSeq);
// typedef GuStringMap PgfCIdMap; // PgfCId -> ?
#define PgfCIdMap GuStringMap			 
typedef PgfCIdMap PgfFlags; // PgfCId -> PgfLiteral
extern GU_DECLARE_TYPE(PgfFlags, GuMap);

extern GU_DECLARE_TYPE(PgfType, struct);
typedef GuVariant PgfProduction;
typedef GuSeq PgfProductions;
extern GU_DECLARE_TYPE(PgfProductions, GuSeq);

typedef struct PgfCatFun PgfCatFun;
typedef GuSeq PgfCatFuns;			      
typedef struct PgfCncCat PgfCncCat;
extern GU_DECLARE_TYPE(PgfCncCat, struct);
typedef GuVariant PgfPatt;
typedef GuSeq PgfPatts;
			      
typedef GuSeq PgfTokens;  // -> PgfToken
extern GU_DECLARE_TYPE(PgfTokens, GuSeq);

bool
pgf_tokens_equal(PgfTokens t1, PgfTokens t2);



typedef PgfExpr PgfTree;

typedef struct PgfEquation PgfEquation;
typedef GuSeq PgfEquations;
typedef PgfEquations PgfEquationsM; // can be null
extern GU_DECLARE_TYPE(PgfEquationsM, GuSeq);
typedef struct PgfCat PgfCat;

typedef PgfSequence PgfSeqId; // shared reference
typedef GuSeq PgfSeqIds;
extern GU_DECLARE_TYPE(PgfSeqId, typedef);

typedef GuSeq PgfSequences;

extern GU_DECLARE_TYPE(PgfSequences, GuSeq);




struct PgfAbstr {
	PgfFlags* aflags;
	PgfCIdMap* funs; // |-> PgfFunDecl*
	PgfCIdMap* cats; // |-> PgfCat*
};

struct PgfPGF {
	uint16_t major_version;
	uint16_t minor_version;
	PgfFlags* gflags;
	PgfCId absname;
	PgfAbstr abstract;
	PgfCIdMap* concretes; // |-> PgfConcr*
	GuPool* pool;
};

extern GU_DECLARE_TYPE(PgfPGF, struct);

struct PgfFunDecl {
	PgfType* type;
	int arity; // Only for computational defs?
	PgfEquationsM defns; // maybe null
	double prob;
};

struct PgfCatFun {
	double prob;
	PgfCId fun; // XXX: resolve to PgfFunDecl*?
};

struct PgfCat {
	// TODO: Add cid here
	PgfHypos context;
	PgfCatFuns functions;  
};


struct PgfCncCat {
	PgfCId cid;
	PgfCCatIds cats;
	PgfFunIds lindefs;
	size_t n_lins;

	GuStrings labels;
	/**< Labels for tuples. All nested tuples, records and tables
	 * in the GF linearization types are flattened into a single
	 * tuple in the corresponding PGF concrete category. This
	 * field holds the labels that indicate which GF field or
	 * parameter (or their combination) each tuple element
	 * represents. */
};

struct PgfCncFun {
	PgfCId fun; // XXX: resolve to PgfFunDecl*?
	PgfSeqIds lins;
};

struct PgfAlternative {
	PgfTokens form;
	/**< The form of this variant as a list of tokens. */

	GuStrings prefixes;
	/**< The prefixes of the following symbol that trigger this
	 * form. */
};

struct PgfCCat {
	PgfCncCat* cnccat;
	PgfProductions prods;
	int fid;
};

extern PgfCCat pgf_ccat_string, pgf_ccat_int, pgf_ccat_float, pgf_ccat_var;

typedef PgfCIdMap PgfPrintNames;
extern GU_DECLARE_TYPE(PgfPrintNames, GuStringMap);

struct PgfConcr {
	PgfFlags* cflags;
	PgfPrintNames* printnames;
	PgfCIdMap* cnccats;
	PgfCCats extra_ccats;
};

extern GU_DECLARE_TYPE(PgfConcr, struct);

typedef enum {
	PGF_SYMBOL_CAT,
	PGF_SYMBOL_LIT,
	PGF_SYMBOL_VAR,
	PGF_SYMBOL_KS,
	PGF_SYMBOL_KP
} PgfSymbolTag;

typedef struct PgfSymbolIdx PgfSymbolIdx;

struct PgfSymbolIdx {
	int d;
	int r;
};

typedef PgfSymbolIdx PgfSymbolCat, PgfSymbolLit, PgfSymbolVar;

typedef struct {
	PgfTokens tokens;
} PgfSymbolKS;

typedef struct PgfSymbolKP
/** A prefix-dependent symbol. The form that this symbol takes
 * depends on the form of a prefix of the following symbol. */
{
	PgfTokens default_form; 
	/**< Default form that this symbol takes if none of of the
	 * variant forms is triggered. */

	PgfAlternatives alts;
	/**< Variant forms whose choise depends on the following
	 * symbol. */
} PgfSymbolKP;




// PgfProduction

typedef enum {
	PGF_PRODUCTION_APPLY,
	PGF_PRODUCTION_COERCE,
} PgfProductionTag;

typedef struct PgfPArg PgfPArg;

struct PgfPArg {
	PgfCCatIds hypos;
	PgfCCatId ccat;
};

GU_DECLARE_TYPE(PgfPArg, struct);

typedef GuSeq PgfPArgs;

GU_DECLARE_TYPE(PgfPArgs, GuSeq);

typedef struct {
	PgfFunId fun; 
	PgfPArgs args;
} PgfProductionApply;

typedef struct PgfProductionCoerce
/** A coercion. This production is a logical union of the coercions of
 * another FId. This allows common subsets of productions to be
 * shared. */
{
	PgfCCatId coerce;
} PgfProductionCoerce;

extern GU_DECLARE_TYPE(PgfProduction, GuVariant);
extern GU_DECLARE_TYPE(PgfBindType, enum);
extern GU_DECLARE_TYPE(PgfLiteral, GuVariant);

bool
pgf_production_eq(PgfProduction p1, PgfProduction p2);


PgfCCatId
pgf_literal_cat(PgfLiteral lit);

// PgfPatt

typedef enum {
	PGF_PATT_APP,
	PGF_PATT_LIT,
	PGF_PATT_VAR,
	PGF_PATT_AS,
	PGF_PATT_WILD,
	PGF_PATT_IMPL_ARG,
	PGF_PATT_TILDE,
	PGF_PATT_NUM_TAGS
} PgfPattTag;

typedef	struct {
	PgfCId ctor;
	PgfPatts args;
} PgfPattApp;

typedef struct {
	PgfLiteral* lit;
} PgfPattLit;

typedef struct {
	PgfCId var;
} PgfPattVar;

typedef struct {
	PgfCId var;
	PgfPatt patt;
} PgfPattAs;

typedef void PgfPattWild;

typedef struct {
	PgfPatt patt;
} PgfPattImplArg;

typedef struct {
	PgfExpr expr;
} PgfPattTilde;

struct PgfEquation {
	PgfPatts patts;
	PgfExpr body;
};



#endif /* PGF_PRIVATE_H_ */
