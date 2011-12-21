#include <gu/variant.h>
#include <gu/map.h>
#include <gu/dump.h>
#include <gu/log.h>
#include <gu/enum.h>
#include <gu/file.h>
#include <pgf/pgf.h>
#include <pgf/data.h>
#include <pgf/parser.h>
#include <pgf/linearize.h>
#include <pgf/expr.h>
#include <pgf/edsl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

int main(int argc, char* argv[]) {
	setlocale(LC_CTYPE, "");
	GuPool* pool = gu_new_pool();
	int status = EXIT_SUCCESS;
	if (argc != 5) {
		fputs("usage: test-translate pgf cat from_lang to_lang\n", stderr);
		status = EXIT_FAILURE;
		goto fail;
	}
	char* filename = argv[1];
	
	GuString cat = gu_str_string(argv[2], pool);
	GuString from_lang = gu_str_string(argv[3], pool);
	GuString to_lang = gu_str_string(argv[4], pool);
	
	FILE* infile = fopen(filename, "r");
	if (infile == NULL) {
		fprintf(stderr, "couldn't open %s\n", filename);
		status = EXIT_FAILURE;
		goto fail;
	}
	GuExn* err = gu_new_exn(NULL, NULL, pool);
	GuIn* in = gu_file_in(infile, pool);
	PgfPGF* pgf = pgf_read(in, pool, err);
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = EXIT_FAILURE;
		goto fail_read;
	}

	PgfConcr* from_concr =
		gu_map_get(pgf->concretes, &from_lang, PgfConcr*);
	PgfConcr* to_concr =
		gu_map_get(pgf->concretes, &to_lang, PgfConcr*);
	PgfParser* parser =
		pgf_new_parser(from_concr, pool);
	PgfLzr* lzr =
		pgf_new_lzr(pgf, to_concr, pool);
	int lin_idx = 0;

	GuOut* out = gu_file_out(stdout, pool);
	// GuWriter* wtr = gu_locale_writer(out, pool);
	GuWriter* wtr = gu_new_utf8_writer(out, pool);
	
	while (true) {
		fprintf(stdout, "> ");
		fflush(stdout);
		char buf[4096];
		char* line = fgets(buf, sizeof(buf), stdin);
		if (line == NULL) {
			if (ferror(stdin)) {
				fprintf(stderr, "Input error\n");
				status = EXIT_FAILURE;
			}
			break;
		} else if (line[0] == '\0') {
			break;
		}
		GuPool* ppool = gu_new_pool();
		PgfParse* parse =
			pgf_parser_parse(parser, cat, lin_idx, pool);
		if (parse == NULL) {
			fprintf(stderr, "Couldn't begin parsing");
			status = EXIT_FAILURE;
			break;
		}
		char* tok = strtok(line, " \n");
		while (tok) {
			GuString tok_s = gu_str_string(tok, pool);
			gu_debug("parsing token \"%s\"", tok);
			parse = pgf_parse_token(parse, tok_s, ppool);
			tok = strtok(NULL, " \n");
		}
		GuEnum* result = pgf_parse_result(parse, ppool);
		PgfExpr expr = gu_next(result, PgfExpr, ppool);
		while (!gu_variant_is_null(expr)) {
			putchar(' ');
			pgf_expr_print(expr, wtr, err);
			putchar('\n');
			GuEnum* cts = pgf_lzr_concretize(lzr, expr, ppool);
			while (true) {
				PgfCncTree ctree = gu_next(cts, PgfCncTree, ppool);
				if (gu_variant_is_null(ctree)) {
					break;
				}
				fputs("  ", stdout);
				pgf_lzr_linearize_simple(lzr, ctree, lin_idx,
							 wtr, err);
				fputc('\n', stdout);
				fflush(stdout);
			}
			expr = gu_next(result, PgfExpr, ppool);
		}
		gu_pool_free(ppool);
	}
fail_read:
	fclose(infile);
fail:
	gu_pool_free(pool);
	return status;
}

