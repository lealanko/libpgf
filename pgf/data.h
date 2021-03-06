// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

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

typedef int PgfLength;
typedef struct GuVariant PgfSymbol;
extern GU_DECLARE_TYPE(PgfSymbol, GuVariant);
typedef struct PgfAlternative PgfAlternative;
typedef GuSeq PgfAlternatives;
typedef struct PgfCncFun PgfCncFun;

typedef int32_t PgfFId;


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
			      
extern GU_DECLARE_TYPE(PgfTokens, GuSeq);


typedef PgfExpr PgfTree;

typedef struct PgfEquation PgfEquation;
typedef GuSeq PgfEquations;
typedef PgfEquations PgfEquationsM; // can be null
extern GU_DECLARE_TYPE(PgfEquationsM, GuSeq);

typedef PgfSequence PgfSeqId; // shared reference
typedef GuSeq PgfSeqIds;
extern GU_DECLARE_TYPE(PgfSeqId, typedef);

typedef GuSeq PgfSequences;

extern GU_DECLARE_TYPE(PgfSequences, GuSeq);


extern GU_DECLARE_KIND(PgfContext);
typedef GuPointerType GuType_PgfContext;
#define GU_TYPE_INIT_PgfContext GU_TYPE_INIT_pointer
typedef PgfPGF* PgfPGFCtx;
typedef PgfAbstr* PgfAbstrCtx;

extern GU_DECLARE_KIND(PgfKey);
typedef GuTypeAlias GuType_PgfKey;
#define GU_TYPE_INIT_PgfKey GU_TYPE_INIT_alias
typedef PgfCId PgfCIdKey;



struct PgfAbstr {
	PgfCId name;
	PgfFlags* aflags;
	PgfCIdMap* funs; // |-> PgfFunDecl*
	PgfCIdMap* cats; // |-> PgfCat*
};

extern GU_DECLARE_TYPE(PgfAbstr, struct);

struct PgfPGF {
	uint16_t major_version;
	uint16_t minor_version;
	PgfFlags* gflags;
	PgfAbstr abstract;
	PgfCIdMap* concretes; // |-> PgfConcr*
	GuPool* pool;
};

struct PgfFunDecl {
	PgfType* type;
	PgfEquationsM defns; // maybe null
	double prob;
	int32_t arity; // Only for computational defs?
};

struct PgfCatFun {
	double prob;
	PgfCId fun; // XXX: resolve to PgfFunDecl*?
};

struct PgfCat {
	PgfPGFCtx pgf;
	PgfCIdKey cid;
	PgfHypos context;
	PgfCatFuns functions;  
};


struct PgfCncCat {
	PgfCId cid;
	PgfCCatIds cats;
	PgfFunIds lindefs;
	size_t n_ctnts;

	GuStrings ctnts;
	/**< Labels for tuples. All nested tuples, records and tables
	 * in the GF linearization types are flattened into a single
	 * tuple in the corresponding PGF concrete category. This
	 * field holds the labels that indicate which GF field or
	 * parameter (or their combination) each tuple element
	 * represents. */
};

PgfCtntId
pgf_cnccat_ctnt_id_(PgfCncCat* cnccat, GuString ctnt);

struct PgfCncFun {
	PgfCId fun; // XXX: resolve to PgfFunDecl*?
	PgfSeqIds lins;
	GU_SIZE_OPTIMIZED(PgfFId fid;,)
};

static inline PgfFId
pgf_cncfun_fid(const PgfCncFun* fun)
{
#ifdef GU_OPTIMIZE_SIZE
	return -1;
#else
	return fun->fid;
#endif
}

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
	// For debugging purposes only.
	GU_SIZE_OPTIMIZED(PgfFId fid;,)
};

#define PGF_INIT_CCAT(cnccat, prods, fid) {			\
		cnccat, prods GU_SIZE_OPTIMIZED(GU_COMMA fid,)	\
			}


static inline PgfFId
pgf_ccat_fid(const PgfCCat* ccat)
{
	return GU_SIZE_OPTIMIZED(ccat->fid, -1);
}

static inline void
pgf_ccat_set_fid(PgfCCat* ccat, PgfFId fid)
{
	GU_SIZE_OPTIMIZED(ccat->fid = fid, (void) (ccat && fid));
}

extern PgfCCat pgf_ccat_string, pgf_ccat_int, pgf_ccat_float, pgf_ccat_var;

typedef PgfCIdMap PgfPrintNames;
extern GU_DECLARE_TYPE(PgfPrintNames, GuStringMap);

typedef GuMap PgfCncCatMap;
extern GU_DECLARE_TYPE(PgfCncCatMap, GuAddrMap);

typedef PgfCat PgfCatId;
extern GU_DECLARE_TYPE(PgfCatId, alias);


struct PgfConcr {
	PgfPGFCtx pgf;
	PgfCIdKey id;
	PgfFlags* cflags;
	PgfPrintNames* printnames;
	PgfCncCatMap* cnccats;
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

// TODO: use uint16_t
struct PgfSymbolIdx {
	int32_t d;
	int32_t r;
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
	PGF_PATT_VAR,
	PGF_PATT_AS,
	PGF_PATT_WILD,
	PGF_PATT_LIT,
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
