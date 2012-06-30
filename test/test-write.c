#include <libgu.h>
#include <locale.h>

int main(void)
{
	setlocale(LC_ALL, "");
	GuPool* pool = gu_local_pool();
	
	GuOut* out = gu_file_out(stdout, pool);
	out = gu_out_buffered(out, pool);
	GuWriter* wtr = gu_new_locale_writer(out, pool);
	wtr = gu_writer_buffered(wtr, pool);
	GuExn* exn = gu_null_exn();
	gu_ucs_write(0xe4, wtr, exn);
	gu_ucs_write(0x0a, wtr, exn);
	gu_writer_flush(wtr, exn);
	gu_pool_free(pool);
	return 0;
}
