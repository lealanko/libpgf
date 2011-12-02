#include <pgf/pgf.h>

#include <gu/dump.h>
#include <gu/file.h>
#include <gu/utf8.h>

int main(void) {
	GuPool* pool = gu_pool_new();
	GuError* err = gu_error(NULL, type, pool);
	GuFile* inf = gu_file(stdin, pool);
	PgfPGF* pgf = pgf_read(&inf->in, pool, err);
	int status = 0;
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = 1;
		goto fail_read;
	}
	GuFile* outf = gu_file(stdout, pool);
	// GuWriter* wtr = gu_locale_writer(&outf->out, pool);
	GuWriter* wtr = gu_utf8_writer(&outf->out, pool);
	GuWriter* bwtr = wtr; // gu_buffered_writer(wtr, 1024, pool);
	GuDump* ctx = gu_new_dump(bwtr, NULL, err, pool);
	gu_dump(gu_type(PgfPGF), pgf, ctx);
	gu_writer_flush(bwtr, err);
fail_read:
	gu_pool_free(pool);
	return status;
}

