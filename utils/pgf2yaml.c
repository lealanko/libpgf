#include <pgf/pgf.h>
#include <pgf/data.h>

int main(void) {
	GError* err = NULL;
	PgfPGF* pgf= pgf_read(stdin, &err);
	if (err != NULL) {
		g_printerr("Reading PGF failed: %s\n", err->message);
		goto fail_read;
	}
	pgf_write_yaml(pgf, stdout, &err);
	if (err != NULL) {
		g_printerr("YAML output failed: %s\n", err->message);
		goto fail_write;
	}
	putchar('\n');
	pgf_free(pgf);
	return 0;
fail_write:
	pgf_free(pgf);
fail_read:
	return 1;
}

