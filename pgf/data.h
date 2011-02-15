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
#include <gu/string.h>
#include <gu/map.h>
#include <gu/type.h>
#include <pgf/pgf.h>

typedef const GuString PgfCId;

typedef struct PgfAbstr PgfAbstr;
typedef struct PgfFunDecl PgfFunDecl;
typedef struct PgfConcr PgfConcr;

typedef int PgfFId;

typedef int PgfLength;

typedef struct GuVariant PgfSymbol;
typedef struct PgfAlternative PgfAlternative;

typedef struct PgfCncFun PgfCncFun;

typedef GuList(PgfSymbol) PgfSequence;



// Reference to PGF.
typedef PgfCncFun** PgfFunId; // key to PgfCncFuns

typedef GuList(PgfCncFun*) PgfCncFuns; 
typedef GuList(PgfFunId) PgfFunIds; 

typedef int PgfId; // second key to PgfConcr.lproductions

typedef GuMap PgfCIdMap; // PgfCId -> ?
typedef PgfCIdMap PgfFlags; // PgfCId -> PgfLiteral

typedef GuVariant PgfExpr;

typedef struct PgfHypo PgfHypo;


typedef struct PgfType PgfType;
extern GU_DECLARE_TYPE(PgfType, struct);


typedef GuVariant PgfProduction;

typedef GuList(PgfProduction) PgfProductions;
			  
typedef struct PgfCncCat PgfCncCat;

typedef GuVariant PgfPatt;

typedef GuString PgfToken;			      

typedef GuStrings PgfTokens;

typedef enum {
	PGF_BIND_TYPE_EXPLICIT,
	PGF_BIND_TYPE_IMPLICIT
} PgfBindType;

extern GU_DECLARE_TYPE(PgfBindType, enum);

struct PgfHypo {
	PgfBindType bindtype;

	PgfCId* cid;
	/**< Locally scoped name for the parameter if dependent types
	 * are used. "_" for normal parameters. */

	PgfType* type;
};

typedef GuList(PgfHypo) PgfHypos;

struct PgfType {
	PgfHypos* hypos;
	PgfCId* cid; /// XXX: resolve to PgfCat*?
	int n_exprs;
	PgfExpr exprs[];
};




typedef int PgfMetaId;

typedef PgfExpr PgfTree;

typedef struct PgfEquation PgfEquation;
typedef GuList(PgfEquation*) PgfEquations;

typedef GuList(PgfCId*) PgfCIds;

typedef struct PgfCat PgfCat;

typedef PgfSequence** PgfSeqId;

typedef GuList(PgfSequence*) PgfSequences;





struct PgfAbstr {
	PgfFlags* aflags;
	PgfCIdMap* funs; // |-> PgfFunDecl*
	PgfCIdMap* cats; // |-> PgfCat*
};

struct PgfPGF {
	uint16_t major_version;
	uint16_t minor_version;
	PgfFlags* gflags;
	PgfCId* absname;
	PgfAbstr abstract;
	PgfCIdMap* concretes; // |-> PgfConcr*
	GuPool* pool;
};

extern GU_DECLARE_TYPE(PgfPGF, struct);

struct PgfFunDecl {
	PgfType* type;
	int arity; // Only for computational defs?
	PgfEquations* defns; // maybe null
};

struct PgfCat {
	PgfHypos* context;
	int n_functions;
	PgfCId* functions[]; // XXX: resolve to PgfFunDecl*?
};

struct PgfCncCat {
	PgfFId first;
	PgfFId last;
	/**< The range of FIds that this concrete category covers. */

	int n_labels; 
	GuString* labels[];
	/**< Labels for tuples. All nested tuples, records and tables
	 * in the GF linearization types are flattened into a single
	 * tuple in the corresponding PGF concrete category. This
	 * field holds the labels that indicate which GF field or
	 * parameter (or their combination) each tuple element
	 * represents. */
};

struct PgfCncFun {
	PgfCId* fun; // XXX: resolve to PgfFunDecl*?
	int n_lins;
	PgfSeqId lins[];
};

struct PgfAlternative {
	PgfTokens* form;
	/**< The form of this variant as a list of tokens. */

	GuStrings* prefixes;
	/**< The prefixes of the following symbol that trigger this
	 * form. */
};



struct PgfConcr {
	PgfFlags* cflags;
	PgfCIdMap* printnames; // GString
	PgfSequences* sequences;
	PgfCncFuns* cncfuns;
	GuIntMap* productions; // |-> PgfProductions*
	GuIntMap* lindefs; // |-> PgfFunIds*
	// Derived from productions
	GuIntMap* pproductions;
	PgfCIdMap* lproductions;

	PgfCIdMap* cnccats;
	int totalcats;
};

GU_DECLARE_TYPE(PgfConcr, struct);

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

typedef struct PgfSymbolIdx PgfSymbolCat;
typedef struct PgfSymbolIdx PgfSymbolLit;
typedef struct PgfSymbolIdx PgfSymbolVar;

typedef struct {
	PgfTokens* tokens;
} PgfSymbolKS;

typedef struct PgfSymbolKP
/** A prefix-dependent symbol. The form that this symbol takes
 * depends on the form of a prefix of the following symbol. */
{
	PgfTokens* default_form; 
	/**< Default form that this symbol takes if none of of the
	 * variant forms is triggered. */

	int n_forms;
	PgfAlternative forms[]; 
	/**< Variant forms whose choise depends on the following
	 * symbol. */
} PgfSymbolKP;




// PgfProduction

typedef enum {
	PGF_PRODUCTION_APPLY,
	PGF_PRODUCTION_COERCE,
	PGF_PRODUCTION_CONST
} PgfProductionTag;

typedef struct PgfPArg PgfPArg;

struct PgfPArg {
	PgfFId fid; // XXX: resolve to PgfProductions*?
	int n_hypos;
	PgfFId hypos[]; // XXX: Change to GuList(PgfFId) since usually empty
};

typedef struct {
	PgfFunId fun; 
	int n_args;
	PgfPArg* args[]; // XXX: Remove indirection once PArg is fixed-length
} PgfProductionApply;

typedef struct PgfProductionCoerce
/** A coercion. This production is a logical union of the coercions of
 * another FId. This allows common subsets of productions to be
 * shared. */
{
	PgfFId coerce; // XXX: resolve to PgfProductions*?
} PgfProductionCoerce;

typedef struct {
	PgfExpr expr; // XXX
	int n_toks;
	GuString* toks[]; // XXX
} PgfProductionConst;


GU_DECLARE_TYPE(PgfProduction, GuVariant);


// PgfLiteral

typedef GuVariant PgfLiteral;

extern GU_DECLARE_TYPE(PgfLiteral, GuVariant);

typedef enum {
	PGF_LITERAL_STR,
	PGF_LITERAL_INT,
	PGF_LITERAL_FLT,
	PGF_LITERAL_NUM_TAGS
} PgfLiteralTag;

typedef struct {
	GuString* val;
} PgfLiteralStr;

typedef struct {
	int val;
} PgfLiteralInt;

typedef struct {
	double val;
} PgfLiteralFlt;


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
	PgfCId* ctor;
	int n_args;
	PgfPatt args[];
} PgfPattApp;

typedef struct {
	PgfLiteral* lit;
} PgfPattLit;

typedef struct {
	PgfCId* var;
} PgfPattVar;

typedef struct {
	PgfCId* var;
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
	PgfExpr body;
	int n_patts;
	PgfPatt patts[];
};



#endif /* PGF_PRIVATE_H_ */
