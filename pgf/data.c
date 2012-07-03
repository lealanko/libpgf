// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include "data.h"
#include "expr.h"
#include <gu/type.h>
#include <gu/variant.h>
#include <gu/assert.h>


PgfCCat pgf_ccat_string = PGF_INIT_CCAT(NULL, GU_NULL_SEQ, -1);
PgfCCat pgf_ccat_int = PGF_INIT_CCAT(NULL, GU_NULL_SEQ, -2);
PgfCCat pgf_ccat_float = PGF_INIT_CCAT(NULL, GU_NULL_SEQ, -3);
PgfCCat pgf_ccat_var = PGF_INIT_CCAT(NULL, GU_NULL_SEQ, -4);

PgfCCatId
pgf_literal_cat(PgfLiteral lit)
{
	switch (gu_variant_tag(lit)) {
	case PGF_LITERAL_STR:
		return &pgf_ccat_string;
	case PGF_LITERAL_INT:
		return &pgf_ccat_int;
	case PGF_LITERAL_FLT:
		return &pgf_ccat_float;
	default:
		gu_impossible();
		return NULL;
	}
}

GU_DEFINE_KIND(PgfContext, reference);
GU_DEFINE_TYPE(PgfPGFCtx, PgfContext, gu_type(PgfPGF));

GU_DEFINE_KIND(PgfKey, alias);
GU_DEFINE_TYPE(PgfCIdKey, PgfKey, gu_type(PgfCId));


GU_DEFINE_TYPE(PgfTokens, GuSeq, gu_type(GuString));

GU_DEFINE_TYPE(PgfCId, typedef, gu_type(GuString));


#define gu_type__PgfCIdMap gu_type__GuStringMap
typedef GuType_GuStringMap GuType_PgfCIdMap;
#define GU_TYPE_INIT_PgfCIdMap GU_TYPE_INIT_GuStringMap

GU_DEFINE_TYPE(PgfCCat, struct,
	       GU_MEMBER_S(PgfCCat, cnccat, PgfCncCat),
	       GU_MEMBER(PgfCCat, prods, PgfProductions));

GU_DEFINE_TYPE(PgfCCatId, shared, gu_type(PgfCCat));

GU_DEFINE_TYPE(PgfCCatIds, GuSeq, gu_type(PgfCCatId));

GU_DEFINE_TYPE(PgfCCats, GuSeq, gu_type(PgfCCatId));

GU_DEFINE_TYPE(PgfAlternative, struct,
	       GU_MEMBER(PgfAlternative, form, PgfTokens),
	       GU_MEMBER(PgfAlternative, prefixes, GuStrings));

GU_DEFINE_TYPE(PgfAlternatives, GuSeq, gu_type(PgfAlternative));

GU_DEFINE_TYPE(PgfSymbolIdx, struct,
	       GU_MEMBER(PgfSymbolIdx, d, int32_t),
	       GU_MEMBER(PgfSymbolIdx, r, int32_t));


GU_DEFINE_TYPE(
	PgfSymbol, GuVariant,
	GU_CONSTRUCTOR(
		PGF_SYMBOL_CAT, PgfSymbolIdx),
	GU_CONSTRUCTOR(
		PGF_SYMBOL_LIT, PgfSymbolIdx),
	GU_CONSTRUCTOR(
		PGF_SYMBOL_VAR, PgfSymbolIdx),
	GU_CONSTRUCTOR_S(
		PGF_SYMBOL_KS, PgfSymbolKS,
		GU_MEMBER(PgfSymbolKS, tokens, PgfTokens)),
	GU_CONSTRUCTOR_S(
		PGF_SYMBOL_KP, PgfSymbolKP,
		GU_MEMBER(PgfSymbolKP, default_form, PgfTokens),
		GU_MEMBER(PgfSymbolKP, alts, PgfAlternatives)));

GU_DEFINE_TYPE(
	PgfCncCat, struct,
	GU_MEMBER(PgfCncCat, cid, PgfCId),
	GU_MEMBER(PgfCncCat, cats, PgfCCatIds),
	GU_MEMBER(PgfCncCat, n_lins, size_t),
	GU_MEMBER(PgfCncCat, lindefs, PgfFunIds),
	GU_MEMBER(PgfCncCat, labels, GuStrings));

// GU_DEFINE_TYPE(PgfSequence, GuList, gu_ptr_type(PgfSymbol));
// GU_DEFINE_TYPE(PgfSequence, GuList, gu_type(PgfSymbol));
GU_DEFINE_TYPE(PgfSequence, GuSeq, gu_type(PgfSymbol));

GU_DEFINE_TYPE(PgfFlags, GuStringMap, gu_type(PgfLiteral), &gu_null_variant);

typedef PgfFlags* PgfFlagsP;

GU_DEFINE_TYPE(PgfFlagsP, pointer, gu_type(PgfFlags));

GU_DEFINE_TYPE(PgfSequences, GuSeq, gu_type(PgfSequence));

GU_DEFINE_TYPE(PgfSeqId, typedef, gu_type(PgfSequence));

GU_DEFINE_TYPE(PgfSeqIds, GuSeq, gu_type(PgfSeqId));

GU_DEFINE_TYPE(
	PgfCncFun, struct,
	GU_MEMBER(PgfCncFun, fun, PgfCId),
	GU_MEMBER(PgfCncFun, lins, PgfSeqIds));

GU_DEFINE_TYPE(PgfCncFuns, GuSeq, 
	       GU_TYPE_LIT(referenced, _, gu_ptr_type(PgfCncFun)));

GU_DEFINE_TYPE(PgfFunId, shared, gu_type(PgfCncFun));

GU_DEFINE_TYPE(PgfFunIds, GuSeq, gu_type(PgfFunId));

GU_DEFINE_TYPE(
	PgfPArg, struct,
	GU_MEMBER(PgfPArg, hypos, PgfCCatIds),
	GU_MEMBER(PgfPArg, ccat, PgfCCatId));

GU_DEFINE_TYPE(PgfPArgs, GuSeq, gu_type(PgfPArg));

GU_DEFINE_TYPE(
	PgfProduction, GuVariant,
	GU_CONSTRUCTOR_S(
		PGF_PRODUCTION_APPLY, PgfProductionApply,
		GU_MEMBER(PgfProductionApply, fun, PgfFunId),
		GU_MEMBER(PgfProductionApply, args, PgfPArgs)),
	GU_CONSTRUCTOR_S(
		PGF_PRODUCTION_COERCE, PgfProductionCoerce,
		GU_MEMBER(PgfProductionCoerce, coerce, PgfCCatId)));

GU_DEFINE_TYPE(PgfProductions, GuSeq, gu_type(PgfProduction));


extern GU_DECLARE_TYPE(PgfPatt, GuVariant);

GU_DEFINE_TYPE(PgfPatts, GuSeq, gu_type(PgfPatt));

GU_DEFINE_TYPE(
	PgfPatt, GuVariant, 
	GU_CONSTRUCTOR_S(
		PGF_PATT_APP, PgfPattApp,
		GU_MEMBER(PgfPattApp, ctor, PgfCId),
		GU_MEMBER(PgfPattApp, args, PgfPatts)),
	GU_CONSTRUCTOR_S(
		PGF_PATT_VAR, PgfPattVar,
		GU_MEMBER(PgfPattVar, var, PgfCId)),
	GU_CONSTRUCTOR_S(
		PGF_PATT_AS, PgfPattAs,
		GU_MEMBER(PgfPattAs, var, PgfCId),
		GU_MEMBER(PgfPattAs, patt, PgfPatt)),
	GU_CONSTRUCTOR(
		PGF_PATT_WILD, void),
	GU_CONSTRUCTOR_S(
		PGF_PATT_LIT, PgfPattLit,
		GU_MEMBER(PgfPattLit, lit, PgfLiteral)),
	GU_CONSTRUCTOR_S(
		PGF_PATT_IMPL_ARG, PgfPattImplArg,
		GU_MEMBER(PgfPattImplArg, patt, PgfPatt)),
	GU_CONSTRUCTOR_S(
		PGF_PATT_TILDE, PgfPattTilde,
		GU_MEMBER(PgfPattTilde, expr, PgfExpr)));

GU_DEFINE_TYPE(
	PgfEquation, struct, 
	GU_MEMBER(PgfEquation, patts, PgfPatts),
	GU_MEMBER(PgfEquation, body, PgfExpr));

// Distinct type so we can give it special treatment in the reader
GU_DEFINE_TYPE(PgfEquationsM, GuSeq, gu_type(PgfEquation));

GU_DEFINE_TYPE(
	PgfFunDecl, struct, 
	GU_MEMBER_P(PgfFunDecl, type, PgfType),
	GU_MEMBER(PgfFunDecl, arity, int32_t),
	GU_MEMBER(PgfFunDecl, defns, PgfEquationsM),
	GU_MEMBER(PgfFunDecl, prob, double));

GU_DEFINE_TYPE(
	PgfCatFun, struct,
	GU_MEMBER(PgfCatFun, prob, double),
	GU_MEMBER(PgfCatFun, fun, PgfCId));

GU_DEFINE_TYPE(PgfCatFuns, GuSeq, gu_type(PgfCatFun));


GU_DEFINE_TYPE(
	PgfCat, struct, 
	GU_MEMBER(PgfCat, pgf, PgfPGFCtx),
	GU_MEMBER(PgfCat, cid, PgfCIdKey),
	GU_MEMBER(PgfCat, context, PgfHypos),
	GU_MEMBER(PgfCat, functions, PgfCatFuns));


GU_DEFINE_TYPE(
	PgfAbstr, struct, 
	GU_MEMBER(PgfAbstr, name, PgfCId),
	GU_MEMBER(PgfAbstr, aflags, PgfFlagsP),
	GU_MEMBER_V(PgfAbstr, funs,
		    GU_TYPE_LIT(pointer, PgfCIdMap*,
				GU_TYPE_LIT(PgfCIdMap, _,
					    gu_ptr_type(PgfFunDecl),
					    &gu_null_struct))),
	GU_MEMBER_V(PgfAbstr, cats,
		    GU_TYPE_LIT(pointer, PgfCIdMap*,
				GU_TYPE_LIT(PgfCIdMap, _,
					    gu_ptr_type(PgfCat),
					    &gu_null_struct))));

GU_DEFINE_TYPE(
	PgfPrintNames, PgfCIdMap, gu_type(GuString), NULL);

GU_DEFINE_TYPE(
	PgfConcr, struct, 
	GU_MEMBER(PgfConcr, cflags, PgfFlagsP),
	GU_MEMBER_P(PgfConcr, printnames, PgfPrintNames),
	GU_MEMBER_V(PgfConcr, cnccats,
		    GU_TYPE_LIT(pointer, PgfCIdMap*,
				GU_TYPE_LIT(PgfCIdMap, _, 
					    gu_ptr_type(PgfCncCat),
					    &gu_null_struct))));

GU_DEFINE_TYPE(
	PgfPGF, referenced,
	GU_TYPE_LIT(struct, PgfPGF,
	GU_MEMBER(PgfPGF, major_version, uint16_t),
	GU_MEMBER(PgfPGF, minor_version, uint16_t),
	GU_MEMBER(PgfPGF, gflags, PgfFlagsP),
	GU_MEMBER(PgfPGF, abstract, PgfAbstr),
	GU_MEMBER_V(PgfPGF, concretes,
		    GU_TYPE_LIT(pointer, PgfCIdMap*,
				GU_TYPE_LIT(PgfCIdMap, _,
					    gu_ptr_type(PgfConcr),
					    &gu_null_struct)))));

