#include <gu/variant.h>
#include <gu/map.h>
#include <gu/dump.h>
#include <gu/log.h>
#include <pgf/pgf.h>
#include <pgf/data.h>
#include <pgf/parser.h>
#include <pgf/linearize.h>
#include <pgf/expr.h>
#include <pgf/edsl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char* argv[]) {
	GuPool* pool = gu_pool_new();
	int status = EXIT_SUCCESS;
	if (argc != 5) {
		fputs("usage: test-translate pgf cat from_lang to_lang\n", stderr);
		status = EXIT_FAILURE;
		goto fail;
	}
	char* filename = argv[1];
	char* cat = argv[2];
	char* from_lang = argv[3];
	char* to_lang = argv[4];
	FILE* infile = fopen(filename, "r");
	if (infile == NULL) {
		fprintf(stderr, "couldn't open %s\n", filename);
		status = EXIT_FAILURE;
		goto fail;
	}
	GuError* err = gu_error_new(pool);
	GuIn* in = gu_in_file(pool, infile);
	PgfPGF* pgf = pgf_read(in, pool, err);
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = EXIT_FAILURE;
		goto fail_read;
	}

	PgfConcr* from_concr = gu_strmap_get(pgf->concretes, from_lang);
	PgfConcr* to_concr = gu_strmap_get(pgf->concretes, to_lang);
	PgfParser* parser = pgf_parser_new(from_concr, pool);
	PgfLzr* lzr = pgf_lzr_new(pool, pgf, to_concr);
	int lin_idx = 0;
	
	while (true) {
		fprintf(stdout, "> ");
		fflush(stdout);
		char buf[4096];
		char* line = fgets(buf, sizeof(buf), stdin);
		if (line == NULL) {
			fprintf(stderr, "Input error");
			status = EXIT_FAILURE;
			break;
		} else if (line[0] == '\0') {
			break;
		}
		GuPool* ppool = gu_pool_new();
		PgfParse* parse = pgf_parser_parse(parser, cat, lin_idx, pool);
		if (parse == NULL) {
			fprintf(stderr, "Couldn't begin parsing");
			status = EXIT_FAILURE;
			break;
		}
		char* tok = strtok(line, " \n");
		while (tok) {
			gu_debug("parsing token \"%s\"", tok);
			parse = pgf_parse_token(parse, tok, ppool);
			tok = strtok(NULL, " \n");
		}
		PgfParseResult* res = pgf_parse_result(parse, ppool);
		PgfExpr expr = pgf_parse_result_next(res, ppool);
		while (!gu_variant_is_null(expr)) {
			putchar(' ');
			pgf_expr_print(expr, stdout);
			putchar('\n');
			PgfLzn* lzn = pgf_lzn_new(lzr, expr, ppool);
			while (true) {
				PgfLinForm form = pgf_lzn_next_form(lzn, ppool);
				if (gu_variant_is_null(form)) {
					break;
				}
				fputs("  ", stdout);
				pgf_lzr_linearize_to_file(lzr, form, lin_idx, stdout);
				fputc('\n', stdout);
				fflush(stdout);
			}
			expr = pgf_parse_result_next(res, ppool);
		}
		gu_pool_free(ppool);
	}
fail_read:
	fclose(infile);
fail:
	gu_pool_free(pool);
	return status;
}

