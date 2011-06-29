#include <pgf/pgf.h>

#include <gu/dump.h>

int main(void) {
	GuPool* pool = gu_pool_new();
	GuError* err = gu_error_new(pool);
	GuIn* in = gu_in_file(pool, stdin);
	PgfPGF* pgf = pgf_read(in, pool, err);
	int status = 0;
	if (!gu_ok(err)) {
		fprintf(stderr, "Reading PGF failed\n");
		status = 1;
		goto fail_read;
	}
	GuDumpCtx* ctx = gu_dump_ctx_new(pool, stdout, NULL);
	gu_dump(gu_type(PgfPGF), pgf, ctx);
	putchar('\n');
fail_read:
	gu_pool_free(pool);
	return status;
}

