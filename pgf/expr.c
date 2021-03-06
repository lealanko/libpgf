// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include "expr.h"
#include <gu/intern.h>
#include <gu/assert.h>
#include <ctype.h>


PgfExpr
pgf_expr_unwrap(PgfExpr expr)
{
	while (true) {
		GuVariantInfo i = gu_variant_open(expr);
		switch (i.tag) {
		case PGF_EXPR_IMPL_ARG: {
			PgfExprImplArg* eimpl = i.data;
			expr = eimpl->expr;
			break;
		}
		case PGF_EXPR_TYPED: {
			PgfExprTyped* etyped = i.data;
			expr = etyped->expr;
			break;
		}
		default:
			return expr;
		}
	}
}

int
pgf_expr_arity(PgfExpr expr)
{
	int n = 0;
	while (true) {
		PgfExpr e = pgf_expr_unwrap(expr);
		GuVariantInfo i = gu_variant_open(e);
		switch (i.tag) {
		case PGF_EXPR_APP: {
			PgfExprApp* app = i.data;
			expr = app->fun;
			n = n + 1;
			break;
		}
		case PGF_EXPR_FUN:
			return n;
		default:
			return -1;
		}
	}
}

PgfApplication*
pgf_expr_unapply(PgfExpr expr, GuPool* pool)
{
	int arity = pgf_expr_arity(expr);
	if (arity < 0) {
		return NULL;
	}
	PgfApplication* appl = gu_new(PgfApplication, pool);
	appl->args = gu_new_seq(PgfExpr, arity, pool);
	for (int n = arity - 1; n >= 0; n--) {
		PgfExpr e = pgf_expr_unwrap(expr);
		gu_assert(gu_variant_tag(e) == PGF_EXPR_APP);
		PgfExprApp* app = gu_variant_data(e);
		gu_seq_set(appl->args, PgfExpr, n, app->arg);
		expr = app->fun;
	}
	appl->fun = pgf_expr_unwrap(expr);
	gu_ensure(gu_variant_tag(appl->fun) == PGF_EXPR_FUN);
	return appl;
}


GU_DEFINE_TYPE(PgfBindType, enum,
	       GU_ENUM_C(PgfBindType, PGF_BIND_TYPE_EXPLICIT),
	       GU_ENUM_C(PgfBindType, PGF_BIND_TYPE_IMPLICIT));

GU_DEFINE_TYPE(PgfLiteral, GuVariant,
	       GU_CONSTRUCTOR_S(PGF_LITERAL_STR, PgfLiteralStr,
				GU_MEMBER(PgfLiteralStr, val, GuString)),
	       GU_CONSTRUCTOR_S(PGF_LITERAL_INT, PgfLiteralInt,
				GU_MEMBER(PgfLiteralInt, val, int)),
	       GU_CONSTRUCTOR_S(PGF_LITERAL_FLT, PgfLiteralFlt,
				GU_MEMBER(PgfLiteralFlt, val, double)));

GU_DECLARE_TYPE(PgfType, struct);

GU_DEFINE_TYPE(PgfHypo, struct,
	       GU_MEMBER(PgfHypo, bindtype, PgfBindType),
	       GU_MEMBER(PgfHypo, cid, PgfCId),
	       GU_MEMBER_P(PgfHypo, type, PgfType));

GU_DEFINE_TYPE(PgfHypos, GuSeq, gu_type(PgfHypo));

GU_DEFINE_TYPE(PgfExprs, GuSeq, gu_type(PgfExpr));

GU_DEFINE_TYPE(PgfType, struct,
	       GU_MEMBER(PgfType, hypos, PgfHypos),
	       GU_MEMBER(PgfType, cid, PgfCId),
	       GU_MEMBER(PgfType, exprs, PgfExprs));

GU_DEFINE_TYPE(
	PgfExpr, GuVariant,
	GU_CONSTRUCTOR_S(
		PGF_EXPR_ABS, PgfExprAbs,
		GU_MEMBER(PgfExprAbs, bind_type, PgfBindType),
		GU_MEMBER(PgfExprAbs, id, PgfCId),
		GU_MEMBER(PgfExprAbs, body, PgfExpr)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_APP, PgfExprApp,
		GU_MEMBER(PgfExprApp, fun, PgfExpr),
		GU_MEMBER(PgfExprApp, arg, PgfExpr)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_LIT, PgfExprLit, 
		GU_MEMBER(PgfExprLit, lit, PgfLiteral)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_META, PgfExprMeta,
		GU_MEMBER(PgfExprMeta, id, int)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_FUN, PgfExprFun,
		GU_MEMBER(PgfExprFun, fun, PgfCId)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_VAR, PgfExprVar,
		GU_MEMBER(PgfExprVar, var, int)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_TYPED, PgfExprTyped,
		GU_MEMBER(PgfExprTyped, expr, PgfExpr),
		GU_MEMBER_P(PgfExprTyped, type, PgfType)),
	GU_CONSTRUCTOR_S(
		PGF_EXPR_IMPL_ARG, PgfExprImplArg,
		GU_MEMBER(PgfExprImplArg, expr, PgfExpr)));

GU_DEFINE_TYPE(
	PgfApplication, struct,
	GU_MEMBER(PgfApplication, fun, PgfExpr),
	GU_MEMBER(PgfApplication, args, PgfExprs));


typedef struct PgfExprParser PgfExprParser;

struct PgfExprParser {
	GuReader* rdr;
	GuIntern* intern;
	GuExn* err;
	GuPool* expr_pool;
	const char* lookahead;
	int next_char; // 0 = EOF
};


static const char pgf_expr_lpar[] = "(";
static const char pgf_expr_rpar[] = ")";
static const char pgf_expr_semic[] = ";";

static char
pgf_expr_parser_next(PgfExprParser* parser)
{
	int next = parser->next_char;
	if (next >= 0) {
		char ret = (char) parser->next_char;
		if (next > 0) {
			parser->next_char = -1;
		}
		return ret;
	}
	char c = gu_getc(parser->rdr, parser->err);
	if (gu_exn_caught(parser->err) == gu_type(GuEOF)) {
		gu_exn_clear(parser->err);
		return '\0';
	}
	return c;
}

static const char*
pgf_expr_parser_lookahead(PgfExprParser* parser)
{
	if (parser->lookahead != NULL) {
		return parser->lookahead;
	}
	const char* str = NULL;
	char c;
	do {
		c = pgf_expr_parser_next(parser);
		if (!gu_ok(parser->err)) {
			return NULL;
		}
	} while (isspace(c));
	switch (c) {
	case '(':
		str = pgf_expr_lpar;
		break;
	case ')':
		str = pgf_expr_rpar;
		break;
	case ';':
		str = pgf_expr_semic;
		break;
	case '\0':
		str = NULL;
		break;
	default:
		if (isalpha(c)) {
			GuPool* tmp_pool = gu_new_pool();
			GuCharBuf* chars = gu_new_buf(char, tmp_pool);
			while (isalnum(c) || c == '_') {
				gu_buf_push(chars, char, c);
				c = pgf_expr_parser_next(parser);
				if (!gu_ok(parser->err)) {
					// A non-EOF I/O error
					return NULL;
				}
			}
			parser->next_char = (unsigned char) c;
			char* tmp_str = gu_chars_str(gu_buf_seq(chars),
						     tmp_pool);
			str = gu_intern_str(parser->intern, tmp_str);
			gu_pool_free(tmp_pool);
		}
	}
	parser->lookahead = str;
	return str;
}

static bool
pgf_expr_parser_token_is_id(const char* str)
{
	if (str == NULL || !str[0]) {
		return false;
	}
	char c = str[0];
	return (isalpha(c) || c == '_');
}

static void
pgf_expr_parser_consume(PgfExprParser* parser)
{
	pgf_expr_parser_lookahead(parser);
	parser->lookahead = NULL;
}

static PgfExpr
pgf_expr_parser_expr(PgfExprParser* parser);

static PgfExpr
pgf_expr_parser_term(PgfExprParser* parser)
{
	const char* la = pgf_expr_parser_lookahead(parser);

	if (la == pgf_expr_lpar) {
		pgf_expr_parser_consume(parser);
		PgfExpr expr = pgf_expr_parser_expr(parser);
		la = pgf_expr_parser_lookahead(parser);
		if (la == pgf_expr_rpar) {
			pgf_expr_parser_consume(parser);
			return expr;
		}
		// Didn't get matching rpar, hence error.
		if (gu_ok(parser->err)) {
			gu_raise(parser->err, PgfReadExn);
		}
		return gu_null_variant;
	} else if (pgf_expr_parser_token_is_id(la)) {
		pgf_expr_parser_consume(parser);
		GuString s = gu_str_string(la, parser->expr_pool);
		return gu_new_variant_i(parser->expr_pool,
					PGF_EXPR_FUN,
					PgfExprFun,
					s);
	}
	return gu_null_variant;
}

static PgfExpr
pgf_expr_parser_expr(PgfExprParser* parser)
{
	PgfExpr expr = pgf_expr_parser_term(parser);
	if (gu_variant_is_null(expr))
	{
		return expr;
	}
	while (true) {
		PgfExpr arg = pgf_expr_parser_term(parser);
		if (gu_variant_is_null(arg)) {
			return expr;
		}
		expr = gu_new_variant_i(parser->expr_pool,
					PGF_EXPR_APP,
					PgfExprApp,
					expr, arg);
	}
}



PgfExpr
pgf_read_expr(GuReader* rdr, GuPool* pool, GuExn* exn)
{
	GuPool* tmp_pool = gu_new_pool();
	GuExn* eof_exn = gu_exn(exn, GuEOF, tmp_pool);
	PgfExprParser* parser = gu_new(PgfExprParser, tmp_pool);
	parser->rdr = rdr;
	parser->intern = gu_new_intern(pool, tmp_pool);
	parser->expr_pool = pool;
	parser->err = eof_exn;
	parser->lookahead = NULL;
	parser->next_char = -1;
	PgfExpr expr = pgf_expr_parser_expr(parser);
	if (!gu_variant_is_null(expr)) {
		if (parser->lookahead == pgf_expr_semic) {
			pgf_expr_parser_consume(parser);
		} else if (parser->lookahead != NULL) {
			gu_raise(exn, PgfReadExn);
		}
	}
	gu_pool_free(tmp_pool);
	return expr;
}

static void
pgf_expr_print_with_paren(PgfExpr expr, bool need_paren,
			  GuWriter* wtr, GuExn* err)
{
	GuVariantInfo ei = gu_variant_open(expr);
	switch (ei.tag) {
	case PGF_EXPR_FUN: {
		PgfExprFun* fun = ei.data;
		gu_string_write(fun->fun, wtr, err);
		break;
	}
	case PGF_EXPR_APP: {
		PgfExprApp* app = ei.data;
		if (need_paren) {
			gu_puts("(", wtr, err);
		}
		pgf_expr_print_with_paren(app->fun, false, wtr, err);
		gu_puts(" ", wtr, err);
		pgf_expr_print_with_paren(app->arg, true, wtr, err);
		if (need_paren) {
			gu_puts(")", wtr, err);
		}
		break;
	}
	case PGF_EXPR_META: {
		PgfExprMeta* meta = ei.data;
		gu_printf(wtr, err, "?%d", meta->id);
		break;
	}
	case PGF_EXPR_ABS:
	case PGF_EXPR_LIT:
	case PGF_EXPR_VAR:
	case PGF_EXPR_TYPED:
	case PGF_EXPR_IMPL_ARG:
		gu_impossible();
		break;
	default:
		gu_impossible();
	}
}

void
pgf_expr_print(PgfExpr expr, GuWriter* wtr, GuExn* err) {
	pgf_expr_print_with_paren(expr, false, wtr, err);
}
