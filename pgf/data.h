#ifndef PGF_DATA_H_
#define PGF_DATA_H_

#include <gu/flex.h>
#include <gu/variant.h>
#include <pgf/pgf.h>

typedef GuString PgfCId;

PgfCId* pgf_cid_new(GuAllocator* ator, gint len);

typedef struct PgfAbstr PgfAbstr;
typedef struct PgfFunDecl PgfFunDecl;
typedef struct PgfConcr PgfConcr;

typedef gint PgfFId;

typedef gint PgfLength;

typedef struct GuVariant PgfSymbol;
typedef struct PgfAlternative PgfAlternative;

typedef struct PgfCncFun PgfCncFun;

typedef GuList(PgfSymbol) PgfSequence;



// Reference to PGF.
typedef PgfCncFun** PgfFunId; // key to PgfCncFuns

typedef GuList(PgfCncFun*) PgfCncFuns; 
typedef GuList(PgfFunId) PgfFunIds; 

typedef gint PgfId; // second key to PgfConcr.lproductions

typedef GHashTable PgfCIdMap; // PgfCId -> ?
typedef PgfCIdMap PgfFlags; // PgfCId -> PgfLiteral

typedef GHashTable PgfIntMap; // gint -> ?


typedef GuVariant PgfExpr;

typedef struct PgfHypo PgfHypo;


typedef struct PgfType PgfType;

typedef GuVariant PgfProduction;

typedef GuList(PgfProduction) PgfProductions;
			  
typedef struct PgfCncCat PgfCncCat;

typedef GuVariant PgfPatt;

typedef GuString PgfToken;			      


typedef enum {
	PGF_BIND_TYPE_EXPLICIT,
	PGF_BIND_TYPE_IMPLICIT
} PgfBindType;

struct PgfHypo {
	PgfBindType bindtype;
	PgfCId* cid;
	PgfType* type;
};

typedef GuList(PgfHypo) PgfHypos;

struct PgfType {
	PgfHypos* hypos;
	PgfCId* cid;
	gint n_exprs;
	PgfExpr exprs[];
};




typedef gint PgfMetaId;

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
	guint16 major_version;
	guint16 minor_version;
	PgfFlags* gflags;
	PgfCId* absname;
	PgfAbstr abstract;
	PgfCIdMap* concretes; // |-> PgfConcr*
	GuMemPool* pool;
};

struct PgfFunDecl {
	PgfType* type;
	gint arity;
	PgfEquations* defns; // maybe null
};

struct PgfCat {
	PgfHypos* context;
	gint n_functions;
	PgfCId* functions[];
};

struct PgfCncCat {
	PgfFId fid1;
	PgfFId fid2;
	gint n_strings; // XXX
	GuString* strings[];
};

struct PgfCncFun {
	PgfCId* fun;
	gint n_lins;
	PgfSeqId lins[];
};

struct PgfAlternative {
	GuStrings* s1; // XXX
	GuStrings* s2; // XXX
};



struct PgfConcr {
	PgfFlags* cflags;
	PgfCIdMap* printnames; // GString
	PgfSequences* sequences;
	PgfCncFuns* cncfuns;
	PgfIntMap* productions; // |-> PgfProductions*
	PgfIntMap* lindefs; // |-> PgfFunIds*
	// Derived from productions
	PgfIntMap* pproductions;
	PgfCIdMap* lproductions;

	PgfCIdMap* cnccats;
	gint totalcats;
};



// PgfSymbol

typedef enum {
	PGF_SYMBOL_CAT,
	PGF_SYMBOL_LIT,
	PGF_SYMBOL_VAR,
	PGF_SYMBOL_KS,
	PGF_SYMBOL_KP
} PgfSymbolTag;

typedef struct {
	gint d;
	gint r;
} PgfSymbolCat;

typedef struct {
	gint d;
	gint r;
} PgfSymbolLit;

typedef struct {
	gint d;
	gint r;
} PgfSymbolVar;

typedef GuStrings PgfSymbolKS;

typedef struct {
	GuStrings* ts;
	gint n_alts;
	PgfAlternative alts[]; // XXX
} PgfSymbolKP;



// PgfProduction

typedef enum {
	PGF_PRODUCTION_APPLY,
	PGF_PRODUCTION_COERCE,
	PGF_PRODUCTION_CONST
} PgfProductionTag;

typedef struct PgfPArg PgfPArg;

struct PgfPArg {
	PgfFId fid;
	gint n_hypos;
	PgfFId hypos[];
};

typedef struct {
	PgfFunId fun; 
	gint n_args;
	PgfPArg* args[];
} PgfProductionApply;

typedef struct {
	PgfFId coerce;
} PgfProductionCoerce;

typedef struct {
	PgfExpr expr; // XXX
	gint n_toks;
	GuString* toks[]; // XXX
} PgfProductionConst;


	


// PgfLiteral

typedef GuVariant PgfLiteral;

typedef enum {
	PGF_LITERAL_STR,
	PGF_LITERAL_INT,
	PGF_LITERAL_FLT,
	PGF_LITERAL_NUM_TAGS
} PgfLiteralTag;

typedef GuString* PgfLiteralStr;
typedef gint PgfLiteralInt;
typedef gdouble PgfLiteralFlt;

// PgfExpr

typedef enum {
	PGF_EXPR_ABS,
	PGF_EXPR_APP,
	PGF_EXPR_LIT,
	PGF_EXPR_META,
	PGF_EXPR_FUN,
	PGF_EXPR_VAR,
	PGF_EXPR_TYPED,
	PGF_EXPR_IMPL_ARG,
	PGF_EXPR_NUM_TAGS
} PgfExprTag;

typedef struct {
	PgfBindType bind_type;
	PgfCId* id;
	PgfExpr body;
} PgfExprAbs;
		
typedef struct {
	PgfExpr fun;
	PgfExpr arg;
} PgfExprApp;

typedef PgfLiteral* PgfExprLit;

typedef PgfMetaId PgfExprMeta;

typedef PgfCId* PgfExprFun;

typedef gint PgfExprVar;

typedef struct {
	PgfExpr expr;
	PgfType* type;
} PgfExprTyped;

typedef PgfExpr PgfExprImplArg;


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
	gint n_args;
	PgfPatt args[];
} PgfPattApp;

typedef PgfLiteral* PgfPattLit;
typedef PgfCId*  PgfPattVar;

typedef struct {
	PgfCId* var;
	PgfPatt patt;
} PgfPattAs;

typedef void PgfPattWild;

typedef PgfPatt PgfPattImplArg;
typedef PgfExpr PgfPattTilde;

struct PgfEquation {
	PgfExpr body;
	gint n_patts;
	PgfPatt patts[];
};



#endif /* PGF_PRIVATE_H_ */
