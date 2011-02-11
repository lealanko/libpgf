#include "data.h"
#include <gu/type.h>
#include <gu/variant.h>

GU_DEFINE_TYPE(GuStringP, pointer, gu_type(GuString));

GU_DEFINE_TYPE(PgfLiteral, GuVariant,
	       GU_CONSTRUCTOR(PGF_LITERAL_STR, GuStringP),
	       GU_CONSTRUCTOR(PGF_LITERAL_INT, int),
	       GU_CONSTRUCTOR(PGF_LITERAL_FLT, double));

GU_DEFINE_TYPE(PgfBindType, enum,
	       GU_ENUM_C(PgfBindType, PGF_BIND_TYPE_EXPLICIT),
	       GU_ENUM_C(PgfBindType, PGF_BIND_TYPE_IMPLICIT));

GU_DECLARE_TYPE(PgfType, struct);

typedef PgfType* PgfTypeP;
GU_DEFINE_TYPE(PgfTypeP, pointer, gu_type(PgfType));

GU_DEFINE_TYPE(PgfHypo, struct,
	       GU_MEMBER(PgfHypo, bindtype, PgfBindType),
	       GU_MEMBER(PgfHypo, cid, GuStringP),
	       GU_MEMBER(PgfHypo, type, PgfTypeP));

GU_DEFINE_TYPE(PgfHypos, GuList, gu_type(PgfHypo));

GU_DECLARE_TYPE(PgfExpr, GuVariant);

GU_DEFINE_TYPE(PgfType, struct,
	       GU_MEMBER_V(PgfType, hypos, gu_ptr_type(PgfHypos)),
	       GU_MEMBER(PgfType, cid, GuStringP),
	       GU_MEMBER(PgfType, n_exprs, GuLength),
	       GU_FLEX_MEMBER(PgfType, exprs, PgfExpr));

GU_DEFINE_TYPE(PgfFId, int, _);
GU_DEFINE_TYPE(PgfMetaId, int, _);
GU_DEFINE_TYPE(PgfId, int, _);

GU_DEFINE_TYPE(
	PgfExpr, GuVariant,
	GU_CONSTRUCTOR_S(
		PGF_EXPR_ABS, PgfExprAbs,
		GU_MEMBER(PgfExprAbs, bind_type, PgfBindType),
		GU_MEMBER(PgfExprAbs, id, GuStringP),
		GU_MEMBER(PgfExprAbs, body, PgfExpr)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_APP, PgfExprApp,
		GU_MEMBER(PgfExprApp, fun, PgfExpr),
		GU_MEMBER(PgfExprApp, arg, PgfExpr)),
	GU_CONSTRUCTOR(PGF_EXPR_LIT, PgfLiteral),
	GU_CONSTRUCTOR(PGF_EXPR_META, int),
	GU_CONSTRUCTOR(PGF_EXPR_FUN, GuStringP),
	GU_CONSTRUCTOR(PGF_EXPR_VAR, int),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_TYPED, PgfExprTyped,
		GU_MEMBER(PgfExprTyped, expr, PgfExpr),
		GU_MEMBER(PgfExprTyped, type, PgfTypeP)),
	GU_CONSTRUCTOR(
		PGF_EXPR_IMPL_ARG, PgfExpr));

GU_DEFINE_TYPE(PgfAlternative, struct,
	       GU_MEMBER(PgfAlternative, form, GuStringsP),
	       GU_MEMBER(PgfAlternative, prefixes, GuStringsP));


GU_DEFINE_TYPE(
	PgfSymbol, GuVariant,
	GU_CONSTRUCTOR_S(
		PGF_SYMBOL_CAT, PgfSymbolCat,
		GU_MEMBER(PgfSymbolCat, d, int),
		GU_MEMBER(PgfSymbolCat, r, int)),
	GU_CONSTRUCTOR_S(
		PGF_SYMBOL_LIT, PgfSymbolLit,
		GU_MEMBER(PgfSymbolLit, d, int),
		GU_MEMBER(PgfSymbolLit, r, int)),
	GU_CONSTRUCTOR_S(
		PGF_SYMBOL_VAR, PgfSymbolVar,
		GU_MEMBER(PgfSymbolVar, d, int),
		GU_MEMBER(PgfSymbolVar, r, int)),
	GU_CONSTRUCTOR(
		PGF_SYMBOL_KS, GuStrings),
	GU_CONSTRUCTOR_S(
		PGF_SYMBOL_KP, PgfSymbolKP,
		GU_MEMBER(PgfSymbolKP, default_form, GuStringsP),
		GU_MEMBER(PgfSymbolKP, n_forms, GuLength),
		GU_MEMBER(PgfSymbolKP, forms, PgfAlternative)));

GU_DEFINE_TYPE(
	PgfCncCat, struct,
	GU_MEMBER(PgfCncCat, first, int),
	GU_MEMBER(PgfCncCat, last, int),
	GU_MEMBER(PgfCncCat, n_labels, GuLength),
	GU_FLEX_MEMBER(PgfCncCat, labels, GuStringP));


// GU_DEFINE_TYPE(PgfSequence, GuList, gu_ptr_type(PgfSymbol));
GU_DEFINE_TYPE(PgfSequence, GuList, gu_type(PgfSymbol));

GU_DEFINE_TYPE(PgfFlags, GuStringMap, 
	       GU_TYPE_LIT(GuVariantAsPtr, gu_type(PgfLiteral)));

typedef PgfFlags* PgfFlagsP;

GU_DEFINE_TYPE(PgfFlagsP, pointer, gu_type(PgfFlags));

GU_DEFINE_TYPE(PgfSequences, GuList,
	       GU_TYPE_LIT(referenced, _, gu_ptr_type(PgfSequence)));

GU_DEFINE_TYPE(PgfSeqId, reference,
	       gu_ptr_type(PgfSequence));

GU_DEFINE_TYPE(
	PgfCncFun, struct,
	GU_MEMBER(PgfCncFun, fun, GuStringP),
	GU_MEMBER(PgfCncFun, n_lins, GuLength),
	GU_FLEX_MEMBER(PgfCncFun, lins, PgfSeqId));

GU_DEFINE_TYPE(PgfCncFuns, GuList, 
	       GU_TYPE_LIT(referenced, _, gu_ptr_type(PgfCncFun)));

GU_DEFINE_TYPE(PgfFunId, reference,
	       gu_ptr_type(PgfCncFun));

GU_DEFINE_TYPE(PgfFunIds, GuList, gu_type(PgfFunId));

GU_DEFINE_TYPE(
	PgfPArg, struct,
	GU_MEMBER(PgfPArg, n_hypos, GuLength),
	GU_MEMBER(PgfPArg, hypos, PgfFId),
	GU_FLEX_MEMBER(PgfPArg, fid, PgfFId));

GU_DEFINE_TYPE(
	PgfProduction, GuVariant,
	GU_CONSTRUCTOR_S(
		PGF_PRODUCTION_APPLY, PgfProductionApply,
		GU_MEMBER(PgfProductionApply, fun, PgfFunId),
		GU_MEMBER(PgfProductionApply, n_args, GuLength),
		GU_FLEX_MEMBER_V(PgfProductionApply, args, 
				 gu_ptr_type(PgfPArg))),
	GU_CONSTRUCTOR_S(
		PGF_PRODUCTION_COERCE, PgfProductionCoerce,
		GU_MEMBER(PgfProductionCoerce, coerce, PgfFId)),
	GU_CONSTRUCTOR_S(
		PGF_PRODUCTION_CONST, PgfProductionConst,
		GU_MEMBER(PgfProductionConst, expr, PgfExpr),
		GU_MEMBER(PgfProductionConst, n_toks, GuLength),
		GU_MEMBER(PgfProductionConst, toks, GuStringP)));

GU_DEFINE_TYPE(PgfProductions, GuList, gu_type(PgfProduction));

GU_DEFINE_TYPE(
	PgfPatt, GuVariant, 
	GU_CONSTRUCTOR_S(
		PGF_PATT_APP, PgfPattApp,
		GU_MEMBER(PgfPattApp, ctor, GuStringP),
		GU_MEMBER(PgfPattApp, n_args, GuLength),
		GU_MEMBER(PgfPattApp, args, PgfPatt)),
	GU_CONSTRUCTOR_V(
		PGF_PATT_LIT, gu_ptr_type(PgfLiteral)),
	GU_CONSTRUCTOR(
		PGF_PATT_VAR, GuStringP),
	GU_CONSTRUCTOR_S(
		PGF_PATT_AS, PgfPattAs,
		GU_MEMBER(PgfPattAs, var, GuStringP),
		GU_MEMBER(PgfPattAs, patt, PgfPatt)),
	GU_CONSTRUCTOR(
		PGF_PATT_WILD, void),
	GU_CONSTRUCTOR(
		PGF_PATT_IMPL_ARG, PgfPatt),
	GU_CONSTRUCTOR(
		PGF_PATT_TILDE, PgfExpr));

GU_DEFINE_TYPE(
	PgfEquation, struct, 
	GU_MEMBER(PgfEquation, body, PgfExpr),
	GU_MEMBER(PgfEquation, n_patts, GuLength),
	GU_MEMBER(PgfEquation, patts, PgfPatt));

GU_DEFINE_TYPE(PgfEquations, GuList, gu_type(PgfEquation));

GU_DEFINE_TYPE(
	PgfFunDecl, struct, 
	GU_MEMBER(PgfFunDecl, type, PgfTypeP),
	GU_MEMBER(PgfFunDecl, arity, int),
	GU_MEMBER_V(PgfFunDecl, defns, gu_ptr_type(PgfEquations)));


GU_DEFINE_TYPE(
	PgfCat, struct, 
	GU_MEMBER_V(PgfCat, context, gu_ptr_type(PgfHypos)),
	GU_MEMBER(PgfCat, n_functions, GuLength),
	GU_FLEX_MEMBER(PgfCat, functions, GuStringP));


GU_DEFINE_TYPE(
	PgfAbstr, struct, 
	GU_MEMBER(PgfAbstr, aflags, PgfFlagsP),
	GU_MEMBER_V(PgfAbstr, funs,
		    GU_TYPE_LIT(pointer, GuStringMap*,
				GU_TYPE_LIT(GuStringMap, _,
					    gu_ptr_type(PgfFunDecl)))),
	GU_MEMBER_V(PgfAbstr, cats,
		    GU_TYPE_LIT(pointer, GuStringMap*,
				GU_TYPE_LIT(GuStringMap, _,
					    gu_ptr_type(PgfCat)))));

GU_DEFINE_TYPE(
	PgfConcr, struct, 
	GU_MEMBER(PgfConcr, cflags, PgfFlagsP),
	GU_MEMBER_V(PgfConcr, printnames,
		    GU_TYPE_LIT(pointer, GuStringMap*,
				GU_TYPE_LIT(GuStringMap, _, 
					    gu_ptr_type(GuString)))),
	GU_MEMBER_V(PgfConcr, sequences, 
		    gu_ptr_type(PgfSequences)),
	GU_MEMBER_V(PgfConcr, cncfuns, 
		    gu_ptr_type(PgfCncFuns)),
	GU_MEMBER_V(PgfConcr, lindefs, 
		    GU_TYPE_LIT(pointer, GuIntMap*,
				GU_TYPE_LIT(GuIntMap, _,
					    gu_ptr_type(PgfFunIds)))),
	GU_MEMBER_V(PgfConcr, productions,
		    GU_TYPE_LIT(pointer, GuIntMap*,
				GU_TYPE_LIT(GuIntMap, _,
					    gu_ptr_type(PgfProductions)))),
	GU_MEMBER_V(PgfConcr, cnccats,
		    GU_TYPE_LIT(pointer, GuStringMap*,
				GU_TYPE_LIT(GuStringMap, _, 
					    gu_ptr_type(PgfCncCat)))),
	GU_MEMBER(PgfConcr, totalcats, PgfFId));

GU_DEFINE_TYPE(
	PgfPGF, struct, 
	GU_MEMBER(PgfPGF, major_version, uint16_t),
	GU_MEMBER(PgfPGF, minor_version, uint16_t),
	GU_MEMBER(PgfPGF, gflags, PgfFlagsP),
	GU_MEMBER(PgfPGF, absname, GuStringP),
	GU_MEMBER(PgfPGF, abstract, PgfAbstr),
	GU_MEMBER_V(PgfPGF, concretes,
		    GU_TYPE_LIT(pointer, GuStringMap*,
				GU_TYPE_LIT(GuStringMap, _,
					    gu_ptr_type(PgfConcr)))));

