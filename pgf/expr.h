#ifndef EXPR_H_
#define EXPR_H_

#include <pgf/data.h>

GU_DECLARE_TYPE(PgfExpr, GuVariant);

typedef GuList(PgfExpr) PgfExprs;


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
	PgfCId* id; // 
	PgfExpr body;
} PgfExprAbs;
		
typedef struct {
	PgfExpr fun;
	PgfExpr arg;
} PgfExprApp;

typedef struct {
	PgfLiteral lit;
} PgfExprLit;

typedef struct {
	PgfMetaId id;
} PgfExprMeta;

typedef struct {
	PgfCId* fun;
} PgfExprFun;

typedef struct {
	int var;
} PgfExprVar;

/**< A variable. The value is a de Bruijn index to the environment,
 * beginning from the innermost variable. */

typedef struct {
	PgfExpr expr;
	PgfType* type;
} PgfExprTyped;

typedef struct {
	PgfExpr expr;
} PgfExprImplArg;

int
pgf_expr_arity(PgfExpr expr);

PgfExpr
pgf_expr_unwrap(PgfExpr expr);

typedef struct PgfApplication PgfApplication;

struct PgfApplication {
	PgfCId* fun;
	int n_args;
	PgfExpr args[];
};

PgfApplication*
pgf_expr_unapply(PgfExpr expr, GuPool* pool);

#endif /* EXPR_H_ */
