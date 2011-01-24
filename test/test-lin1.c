#include <gu/variant.h>
#include <gu/map.h>
#include <pgf/pgf.h>
#include "pgf/data.h"
#include "pgf/linearize.h"

#include <stdio.h>
int main(void) {
	GuPool* pool = gu_pool_new();

	GError* err = NULL;
	PgfPGF* pgf = pgf_read(stdin, &err);
	if (err != NULL) {
		g_printerr("Reading PGF failed: %s\n", err->message);
		goto fail_read;
	}

	PgfCId* lang = gu_string_cstr_new(pool, "Test1Cnc");
	PgfConcr* concr = gu_map_get(pgf->concretes, lang);
	PgfLinearizer* lzr = pgf_linearizer_new(pool, pgf, concr);
	PgfLinearization* lzn = pgf_lzn_new(pool, lzr);

	PgfExpr expr;
	PgfExprFun* fun = gu_variant_new(pool, PGF_EXPR_FUN, PgfExprFun, &expr);
	*fun = gu_string_cstr_new(pool, "a");

	pgf_lzn_linearize_to_file(lzn, expr, 0, 0, stdout);
	return 0;
fail_write:
	pgf_free(pgf);
fail_read:
	return 1;
}

