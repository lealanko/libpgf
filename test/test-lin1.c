#include <gu/variant.h>
#include <gu/map.h>
#include <gu/dump.h>
#include <pgf/pgf.h>
#include "pgf/data.h"
#include "pgf/linearize.h"
#include <pgf/edsl.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {
	GuPool* pool = gu_pool_new();
	
	if (argc != 3) {
		fputs("usage: test-lin1 Grammar.pgf lang\n", stderr);
		return EXIT_FAILURE;
	}
	char* filename = argv[1];
	char* lang = argv[2];
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
	PgfLzr* lzr = pgf_lzr_new(pool, pgf, concr);

	GuDumpCtx* ctx = gu_dump_ctx_new(pool, stdout, NULL);
	ctx->print_address = true;
	// gu_dump(gu_type(PgfLinearizer), lzr, ctx);


#define PGF_EDSL_POOL pool
	// PgfExpr expr = APPV(s, APPV(s, VAR(o)));
	// PgfExpr expr = VAR(Pizza);
	// PgfExpr expr = APPV2(Is, APPV(This, VAR(Pizza)), APPV(PropQuality, VAR(Delicious)));
	// PgfExpr expr = APPV2(Is, APPV(This, APPV2(QKind, VAR(Warm), VAR(Pizza))), VAR(Delicious));
	// PgfExpr expr = APPV2(Is, APPV(This, APPV2(QKind, APPV(Very, VAR(Expensive)), VAR(Wine))), VAR(Boring));
	// PgfExpr expr = APPV(Very, VAR(Delicious));
	// PgfExpr expr = APPV(This, VAR(Pizza));
	// PgfExpr expr = APPV2(QKind, VAR(Warm), VAR(Pizza));
	// PgfExpr expr = VAR(Warm);
	// PgfExpr expr = APPV(PQuestion, APPV(WherePerson, VAR(YouFamMale)));

	while (true) {
		fprintf(stdout, "> ");
		fflush(stdout);
		PgfExpr expr = pgf_expr_parse(stdin, pool);
		
		if (gu_variant_is_null(expr)) {
			break;
		}
	
		PgfLzn* lzn = pgf_lzn_new(lzr, expr, pool);
		
		while (true) {
			PgfLinForm form = pgf_lzn_next_form(lzn, pool);
			if (gu_variant_is_null(form)) {
				break;
			}
			pgf_lzr_linearize_to_file(lzr, form, 0, stdout);
			fputc('\n', stdout);
			fflush(stdout);
		}
	}
	return 0;
fail_write:
	pgf_free(pgf);
fail_read:
	return 1;
}

