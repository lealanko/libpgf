#include <gu/file.h>

typedef struct GuFileWriter GuFileWriter;

static void
gu_file_write(GuWriter* wtr, const wchar_t* wcs, size_t size, GuError* err)
{
	GuFile* file = gu_container(wtr, GuFile, wtr);
	size_t n = 0;
	errno = 0;
	while (n < size) {
		const wchar_t* next = wmemchr(&wcs[n], L'\0', size - n);
		if (next == NULL) {
			break;
		}
		if (fputws(&wcs[n], file->file) < 0 ||
		    fputwc(L'\0', file->file) == WEOF) {
			goto fail;
		}
		n = next + 1 - wcs;
	}
	while (n < size) {
		if (fputwc(wcs[n++], file->file) == WEOF) {
			goto fail;
		}
	}
	return;

fail:
	gu_raise_errno(err);
}

static size_t
gu_file_read(GuReader* rdr, wchar_t* buf, size_t len, GuError* err)
{
	GuFile* file = gu_container(rdr, GuFile, rdr);
	errno = 0;
	for (size_t i = 0; i < len; i++) {
		wint_t w = fgetwc(file->file);
		if (w == WEOF) {
			if (ferror(file->file)) {
				gu_raise_errno(err);
			}
			return i;
		}
		buf[i] = (wchar_t) w;
	}
	return len;
}

static size_t 
gu_file_input(GuIn* in, uint8_t* buf, size_t max_len, GuError* err)
{
	GuFile* file = gu_container(in, GuFile, in);
	errno = 0;
	size_t got = fread(buf, 1, max_len, file->file);
	if (got == 0) {
		if (ferror(file->file)) {
			gu_raise_errno(err);
		}
	}
	return got;
}

static size_t
gu_file_output(GuOut* out, const uint8_t* buf, size_t len, GuError* err)
{
	GuFile* file = gu_container(out, GuFile, out);
	errno = 0;
	size_t wrote = fwrite(buf, 1, len, file->file);
	if (wrote < len) {
		if (ferror(file->file)) {
			gu_raise_errno(err);
		}
	}
	return wrote;
}


GuFile*
gu_file(FILE* file, GuPool* pool)
{
	return gu_new_s(pool, GuFile,
			.file = file,
			.wtr = { gu_file_write },
			.rdr = { gu_file_read },
			.in = { gu_file_input },
			.out = { gu_file_output });
}

