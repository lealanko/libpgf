#include <gu/variant.h>
#include <gu/map.h>
#include <gu/dump.h>
#include <gu/file.h>
#include <pgf/pgf.h>
#include "pgf/data.h"
#include "pgf/linearize.h"
#include <pgf/edsl.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {
	GuPool* pool = gu_pool_new();
	int status = EXIT_SUCCESS;
	if (argc != 3) {
		fputs("usage: test-lin1 Grammar.pgf lang\n", stderr);
		status = EXIT_FAILURE;
		goto fail;
	}
	char* filename = argv[1];
	char* lang = argv[2];
	FILE* infile = fopen(filename, "r");
	if (infile == NULL) {
		fprintf(stderr, "couldn't open %s\n", filename);
		status = EXIT_FAILURE;
		goto fail;
	}
	GuError* err = gu_new_error(NULL, gu_kind(GuEOF), pool);
	GuFile* inf = gu_file(infile, pool);
	PgfPGF* pgf = pgf_read(&inf->in, pool, err);
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = EXIT_FAILURE;
		goto fail;
	}
	GuString lang_s = gu_str_string(lang, pool);
	PgfConcr* concr = gu_map_get(pgf->concretes, &lang_s, PgfConcr*);
	PgfLzr* lzr = pgf_lzr_new(pool, pgf, concr);

	GuFile* outf = gu_file(stdout, pool);
	GuWriter* wtr = gu_locale_writer(&outf->out, pool);
	GuDump* ctx = gu_new_dump(wtr, NULL, err, pool);
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

	GuFile* sin = gu_file(stdin, pool);
	GuReader* rdr = gu_char_reader(&sin->in, pool);
	
	while (true) {
		gu_puts("> ", wtr, err);
		gu_writer_flush(wtr, err);
		PgfExpr expr = pgf_read_expr(rdr, pool, err);
		
		if (gu_variant_is_null(expr)) {
			break;
		}
	
		PgfLzn* lzn = pgf_lzn_new(lzr, expr, pool);
		
		while (true) {
			PgfLinForm form = pgf_lzn_next_form(lzn, pool);
			if (gu_variant_is_null(form)) {
				break;
			}
			int n = pgf_lin_form_n_fields(form);
			for (int i = 0; i < n; i++) {
				pgf_lzr_linearize_simple(lzr, form, i,
							 wtr, err);
				gu_putc('\n', wtr, err);
				gu_writer_flush(wtr, err);
			}
			gu_puts(".\n", wtr, err);
			gu_writer_flush(wtr, err);
		}
	}
fail:
	gu_pool_free(pool);
	return status;
}

