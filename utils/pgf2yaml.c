#include <pgf/pgf.h>

#include <gu/dump.h>
#include <gu/file.h>
#include <gu/utf8.h>

int main(void) {
	GuPool* pool = gu_pool_new();
	GuError* err = gu_error(NULL, type, pool);
	GuIn* in = gu_file_in(stdin, pool);
	PgfPGF* pgf = pgf_read(in, pool, err);
	int status = 0;
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = 1;
		goto fail_read;
	}
	GuFile* outf = gu_file(stdout, pool);
	// GuWriter* wtr = gu_locale_writer(&outf->out, pool);
	gu_out_enable_buffering(&outf->out, 4096, pool);
	GuWriter* wtr = gu_make_utf8_writer(&outf->out, pool);
	GuWriter* bwtr = wtr;
	GuDump* ctx = gu_new_dump(bwtr, NULL, err, pool);
	gu_dump(gu_type(PgfPGF), pgf, ctx);
	gu_writer_flush(bwtr, err);
fail_read:
	gu_pool_free(pool);
	return status;
}

