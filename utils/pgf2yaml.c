#include <pgf/pgf.h>

#include <gu/dump.h>
#include <gu/file.h>

int main(void) {
	GuPool* pool = gu_pool_new();
	GuError* err = gu_error_new(pool);
	GuFile* inf = gu_file(stdin, pool);
	PgfPGF* pgf = pgf_read(&inf->in, pool, err);
	int status = 0;
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = 1;
		goto fail_read;
	}
	GuFile* outf = gu_file(stdout, pool);
	GuDumpCtx* ctx = gu_dump_ctx_new(pool, &outf->wtr, NULL);
	gu_dump(gu_type(PgfPGF), pgf, ctx);
	putchar('\n');
fail_read:
	gu_pool_free(pool);
	return status;
}

