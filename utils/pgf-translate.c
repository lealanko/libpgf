// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#define _POSIX_C_SOURCE 2 // For getopt

#include <libpgf.h>
#include <gu/file.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
	GuString catname;
	GuString from_ctnt;
	GuString to_ctnt;
	bool show_expr;
	const char* filename;
	GuString from;
	GuString to;
} Options;


Options*
parse_options(int argc, char* argv[], GuPool* pool, GuExn* exn)
{
	Options opts = { gu_null_string };
	int opt;
	while ((opt = getopt(argc, argv, "c:F:T:t")) != -1) {
		GuString* dst = NULL;
		switch (opt) {
		case 'c':
			dst = &opts.catname;
			break;
		case 'F':
			dst = &opts.from_ctnt;
			break;
		case 'T':
			dst = &opts.to_ctnt;
			break;
		case 't':
			opts.show_expr = true;
			break;
		default:
			gu_raise(exn, void);
			return NULL;
		}
		if (dst) {
			*dst = gu_str_string(optarg, pool);
		}
	}
	if (optind != argc - 3) {
		gu_raise(exn, void);
		return NULL;
	}
	opts.filename = argv[optind];
	opts.from = gu_str_string(argv[optind + 1], pool);
	opts.to = gu_str_string(argv[optind + 2], pool);
	Options* ret = gu_new(Options, pool);
	*ret = opts;
	return ret;
}


PgfPGF*
read_pgf(const char* filename, GuPool* opool, GuExn* exn)
{
	FILE* infile = fopen(filename, "r");
	if (infile == NULL) {
		gu_raise_i(exn, GuStr, "couldn't open file");
		return NULL;
	}
	GuPool* pool = gu_local_pool();
	// Create an input stream from the input file
	GuIn* in = gu_file_in(infile, pool);
	// Read the PGF grammar.
	PgfPGF* pgf = pgf_read_pgf(in, opool, exn);
	gu_pool_free(pool);
	fclose(infile);
	return pgf;
}


void
doit(PgfPGF* pgf, const Options* opts, GuPool* pool, GuExn* exn)
{
	PgfCat* cat = pgf_pgf_cat(pgf, opts->catname);
	if (!cat) {
		gu_raise_i(exn, GuStr, "Bad -c option, or no startcat in PGF");
		return;
	}
	PgfConcr* from_concr = pgf_pgf_concr(pgf, opts->from, pool);
	if (!from_concr) {
		from_concr = pgf_pgf_concr_by_lang(pgf, opts->from, pool);
	}
	if (!from_concr) {
		gu_raise_i(exn, GuStr, "Unknown source language");
		return;
	}

	PgfConcr* to_concr = pgf_pgf_concr(pgf, opts->to, pool);
	if (!to_concr) {
		to_concr = pgf_pgf_concr_by_lang(pgf, opts->to, pool);
	}
	if (!to_concr) {
		gu_raise_i(exn, GuStr, "Unknown target language");
		return;
	}

	PgfCtntId from_ctnt =
		pgf_concr_cat_ctnt_id(from_concr, cat, opts->from_ctnt);
	if (from_ctnt == PGF_CTNT_ID_BAD) {
		gu_raise_i(exn, GuStr, "Bad source constituent");
		return;
	}

	PgfCtntId to_ctnt =
		pgf_concr_cat_ctnt_id(to_concr, cat, opts->to_ctnt);
	if (to_ctnt == PGF_CTNT_ID_BAD) {
		gu_raise_i(exn, GuStr, "Bad target constituent");
		return;
	}

	// Create the parser for the source category
	PgfParser* parser = pgf_new_parser(from_concr, pool);

	// Create a linearizer for the destination category
	PgfLzr* lzr = pgf_new_lzr(to_concr, pool);

	// Create an output stream for stdout
	GuOut* out = gu_file_out(stdout, pool);

	// Use a writer that translates characters into the current locale's
	// encoding.
	GuWriter* wtr = gu_new_locale_writer(out, pool);

	// The interactive translation loop.
	// XXX: This currently reads stdin directly, so it doesn't support
	// encodings properly. TODO: use a locale reader for input
	while (gu_ok(exn)) {
		fprintf(stdout, "> ");
		fflush(stdout);
		char buf[4096];
		char* line = fgets(buf, sizeof(buf), stdin);
		if (line == NULL) {
			if (ferror(stdin)) {
				gu_raise_i(exn, GuStr, "Input Error");
			}
			break;
		} else if (line[0] == '\0') {
			// End nicely on empty input
			break;
		}
		// We create a temporary pool for translating a single
		// sentence, so our memory usage doesn't increase over time.
		GuPool* ppool = gu_local_pool();

		// Begin parsing a sentence of the specified category
		PgfParse* parse =
			pgf_parser_parse(parser, cat, from_ctnt, ppool);
		if (parse == NULL) {
			gu_raise_i(exn, GuStr, "Couldn't begin parsing");
			goto end_loop;
		}

		// Just do utterly naive space-separated tokenization
		char* tok = strtok(line, " \n");
		while (tok) {
			GuString tok_s = gu_str_string(tok, pool);
			gu_debug("parsing token \"%s\"", tok);
			// feed the token to get a new parse state
			parse = pgf_parse_token(parse, tok_s, ppool);
			if (!parse) {
				gu_raise_i(exn, GuStr, "Unexpected token");
				goto end_loop;
			}
			tok = strtok(NULL, " \n");
		}

		// Now begin enumerating the resulting syntax trees
		GuEnum* result = pgf_parse_result(parse, ppool);
		PgfExpr expr;
		while (gu_enum_next(result, &expr, ppool)) {
			if (opts->show_expr) {
				// Write out the abstract syntax tree
				gu_putc(' ', wtr, exn);
				pgf_expr_print(expr, wtr, exn);
				gu_putc('\n', wtr, exn);
			}
			if (!gu_ok(exn)) goto end_loop;
			// Enumerate the concrete syntax trees corresponding
			// to the abstract tree.
			GuEnum* cts = pgf_lzr_concretize(lzr, expr, ppool);
			PgfCncTree ctree;
			while (gu_enum_next(cts, &ctree, ppool)) {
				gu_puts("  ", wtr, exn);
				// Linearize the concrete tree as a simple
				// sequence of strings.
				pgf_lzr_linearize_simple(lzr, ctree, to_ctnt,
							 wtr, exn);
				if (!gu_ok(exn)) goto end_loop;
				gu_putc('\n', wtr, exn);
				gu_writer_flush(wtr, exn);
			}
		}
	end_loop:
		gu_pool_free(ppool);
	}
}

static void usage(const char* progname)
{
	fprintf(stdout, "\
Usage: %s [OPTIONS]... PGF-FILE FROM TO\n\
Translate phrases of PGF grammar PGF-FILE from FROM to TO, where FROM and TO\n\
are either languages or names of concrete grammars.\n\
\n\
Options:\n\
	-c CAT	Translate from category CAT instead of the default category\n\
	-t	Show abstract syntax expressions\n\
	-F CTNT	Parse from constituent CTNT\n\
	-T CTNT	Linearize to constituent CTNT\n\
", progname);

}

int main(int argc, char* argv[])
{
	// Set the character locale, so we can produce proper output.
	setlocale(LC_CTYPE, "");
	GuPool* pool = gu_new_pool();
	// Create an exception frame that catches all errors.
	GuExn* exn = gu_top_exn(pool);
	Options* opts = parse_options(argc, argv, pool, exn);
	if (!gu_ok(exn)) {
		usage(argv[0]);
		goto end;
	}
	PgfPGF* pgf = read_pgf(opts->filename, pool, exn);
	if (!gu_ok(exn)) goto end;
	doit(pgf, opts, pool, exn);
	if (!gu_ok(exn)) goto end;
end:;
	int status = EXIT_FAILURE;
	GuType* exn_type = gu_exn_caught(exn);
	if (!exn_type) {
		status = EXIT_SUCCESS;
	} else if (exn_type == gu_type(GuStr)) {
		const GuStr* strp = gu_exn_caught_data(exn);
		fprintf(stderr, "Error: %s\n", *strp);
	} else {
		fprintf(stderr, "Error\n");
	} 
	gu_pool_free(pool);
	return status;
}
