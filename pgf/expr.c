#include "expr.h"
#include <gu/intern.h>
#include <gu/seq.h>
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
	PgfApplication* appl = gu_flex_new(pool, PgfApplication, args, arity);
	appl->n_args = arity;
	for (int n = arity - 1; n >= 0; n--) {
		PgfExpr e = pgf_expr_unwrap(expr);
		gu_assert(gu_variant_tag(e) == PGF_EXPR_APP);
		PgfExprApp* app = gu_variant_data(e);
		appl->args[n] = app->arg;
		expr = app->fun;
	}
	PgfExpr e = pgf_expr_unwrap(expr);
	gu_assert(gu_variant_tag(e) == PGF_EXPR_FUN);
	PgfExprFun* fun = gu_variant_data(e);
	appl->fun = fun->fun;
	return appl;
}


GU_DEFINE_TYPE(
	PgfExpr, GuVariant,
	GU_CONSTRUCTOR_S(
		PGF_EXPR_ABS, PgfExprAbs,
		GU_MEMBER(PgfExprAbs, bind_type, PgfBindType),
		GU_MEMBER_P(PgfExprAbs, id, GuString),
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
		GU_MEMBER_P(PgfExprFun, fun, GuString)),
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


typedef struct PgfExprParser PgfExprParser;

struct PgfExprParser {
	FILE* input;
	GuIntern* intern;
	GuPool* expr_pool;
	GuCString* lookahead;
};

static GU_DEFINE_ATOM(pgf_expr_lpar, "(");
static GU_DEFINE_ATOM(pgf_expr_rpar, ")");
static GU_DEFINE_ATOM(pgf_expr_semic, ";");

static GuCString*
pgf_expr_parser_lookahead(PgfExprParser* parser)
{
	if (parser->lookahead != NULL) {
		return parser->lookahead;
	}
	GuCString* str = NULL;
	int c;
	do {
		c = fgetc(parser->input);
	} while (isspace(c));
	switch (c) {
	case '(':
		str = gu_atom(pgf_expr_lpar);
		break;
	case ')':
		str = gu_atom(pgf_expr_rpar);
		break;
	case ';':
		str = gu_atom(pgf_expr_semic);
		break;
	default:
		if (isalpha(c)) {
			GuPool* tmp_pool = gu_pool_new();
			GuCharSeq* charq = gu_char_seq_new(tmp_pool);
			while (isalnum(c) || c == '_') {
				gu_char_seq_push(charq, c);
				c = fgetc(parser->input);
			}
			if (c != EOF) {
				ungetc(c, parser->input);
			}
			GuString* tmp_str = gu_char_seq_to_string(charq, tmp_pool);
			str = gu_intern_string(parser->intern, tmp_str);
			gu_pool_free(tmp_pool);
		}
	}
	parser->lookahead = str;
	return str;
}

static bool
pgf_expr_parser_token_is_id(GuCString* str)
{
	if (str == NULL || gu_string_length(str) == 0) {
		return false;
	}
	char c = gu_string_cdata(str)[0];
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
	GuCString* la = pgf_expr_parser_lookahead(parser);

	if (la == gu_atom(pgf_expr_lpar)) {
		pgf_expr_parser_consume(parser);
		PgfExpr expr = pgf_expr_parser_expr(parser);
		la = pgf_expr_parser_lookahead(parser);
		if (la == gu_atom(pgf_expr_rpar)) {
			pgf_expr_parser_consume(parser);
			return expr;
		}
	} else if (pgf_expr_parser_token_is_id(la)) {
		pgf_expr_parser_consume(parser);
		return gu_variant_new_i(parser->expr_pool,
					PGF_EXPR_FUN,
					PgfExprFun,
					la);
	}
	return gu_variant_null;
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
		expr = gu_variant_new_i(parser->expr_pool,
					PGF_EXPR_APP,
					PgfExprApp,
					expr, arg);
	}
}



PgfExpr
pgf_expr_parse(FILE* input, GuPool* pool)
{
	GuPool* tmp_pool = gu_pool_new();
	PgfExprParser* parser = gu_new(tmp_pool, PgfExprParser);
	parser->input = input;
	parser->intern = gu_intern_new(tmp_pool, pool);
	parser->expr_pool = pool;
	parser->lookahead = NULL;
	PgfExpr expr = pgf_expr_parser_expr(parser);
	GuCString* la = pgf_expr_parser_lookahead(parser);
	if (la == gu_atom(pgf_expr_semic)) {
		pgf_expr_parser_consume(parser);
	} else {
		expr = gu_variant_null;
	}
	gu_pool_free(tmp_pool);
	return expr;
}

static void
pgf_expr_print_with_paren(PgfExpr expr, bool need_paren, FILE* out)
{
	GuVariantInfo ei = gu_variant_open(expr);
	switch (ei.tag) {
	case PGF_EXPR_FUN: {
		PgfExprFun* fun = ei.data;
		fprintf(out, GU_STRING_FMT, GU_STRING_FMT_ARGS(fun->fun));
		break;
	}
	case PGF_EXPR_APP: {
		PgfExprApp* app = ei.data;
		if (need_paren) {
			fprintf(out, "(");
		}
		pgf_expr_print_with_paren(app->fun, false, out);
		fprintf(out, " ");
		pgf_expr_print_with_paren(app->arg, true, out);
		if (need_paren) {
			fprintf(out, ")");
		}
		break;
	}
	case PGF_EXPR_ABS:
	case PGF_EXPR_LIT:
	case PGF_EXPR_META:
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
pgf_expr_print(PgfExpr expr, FILE* out) {
	pgf_expr_print_with_paren(expr, false, out);
}
