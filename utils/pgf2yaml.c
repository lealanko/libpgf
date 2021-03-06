// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <pgf/pgf.h>
#include <pgf/reader.h>

#include <gu/dump.h>
#include <gu/file.h>
#include <gu/utf8.h>

int main(void) {
	GuPool* pool = gu_new_pool();
	GuExn* err = gu_exn(NULL, type, pool);
	GuIn* in = gu_file_in(stdin, pool);
	PgfPGF* pgf = pgf_read_pgf(in, pool, err);
	int status = 0;
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = 1;
		goto fail_read;
	}
	GuOut* out = gu_file_out(stdout, pool);
	GuOut* bout = gu_out_buffered(out, pool);
	// GuWriter* wtr = gu_locale_writer(bout, pool);
	GuWriter* wtr = gu_new_utf8_writer(bout, pool);
	GuDump* ctx = gu_new_dump(wtr, NULL, err, pool);
	gu_dump(gu_type(PgfPGF), pgf, ctx);
	gu_writer_flush(wtr, err);
fail_read:
	gu_pool_free(pool);
	return status;
}

