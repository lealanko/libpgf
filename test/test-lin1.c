#include <gu/variant.h>
#include <gu/map.h>
#include <gu/dump.h>
#include <pgf/pgf.h>
#include "pgf/data.h"
#include "pgf/linearize.h"
#include <pgf/expr.h>
#include <stdio.h>
#include <stdlib.h>

GuTypeTable dump_table = GU_TYPETABLE(
	GU_SLIST(GuTypeTable*,
		 &gu_dump_table,
		 &pgf_linearize_dump_table),
	{ NULL, NULL });

int main(int argc, char* argv[]) {
	GuPool* pool = gu_pool_new();
	
	if (argc != 4) {
		fputs("usage: test-lin1 Grammar.pgf lang rootsym\n", stderr);
		return EXIT_FAILURE;
	}
	char* filename = argv[1];
	char* lang = argv[2];
	char* rootsym = argv[3];

	FILE* in = fopen(filename, "r");
	if (in == NULL) {
		fprintf(stderr, "couldn't open %s\n", filename);
		return EXIT_FAILURE;
	}
	GError* err = NULL;
	PgfPGF* pgf = pgf_read(in, &err);
	if (err != NULL) {
		g_printerr("Reading PGF failed: %s\n", err->message);
		goto fail_read;
	}

	PgfCId* lang_s = gu_string_new_c(pool, lang);
	PgfConcr* concr = gu_map_get(pgf->concretes, lang_s);
	PgfLinearizer* lzr = pgf_linearizer_new(pool, pgf, concr);

	GuDumpCtx* ctx = gu_dump_ctx_new(pool, stdout, &dump_table);
	ctx->print_address = true;
	gu_dump(gu_type(PgfLinearizer), lzr, ctx);

	PgfLinearization* lzn = pgf_lzn_new(pool, lzr);

#define PGF_EXPR_POOL pool
	// PgfExpr expr = APPV(s, APPV(s, VAR(o)));
	// PgfExpr expr = VAR(Pizza);
	// PgfExpr expr = APPV2(Is, APPV(This, VAR(Pizza)), VAR(Delicious));
	PgfExpr expr = APPV2(Is, APPV(This, APPV2(QKind, VAR(Warm), VAR(Pizza))), VAR(Delicious));
	// PgfExpr expr = APPV(Very, VAR(Delicious));
	// PgfExpr expr = APPV2(QKind, VAR(Warm), VAR(Pizza));
	// PgfExpr expr = VAR(Warm);

	while (pgf_lzn_linearize_to_file(lzn, expr, -1, 0, stdout));
	fputc('\n', stdout);
	return 0;
fail_write:
	pgf_free(pgf);
fail_read:
	return 1;
}

