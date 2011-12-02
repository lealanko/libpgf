#include <gu/file.h>

typedef struct GuFileWriter GuFileWriter;

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

static void
gu_file_flush(GuOut* out, GuError* err)
{
	GuFile* file = gu_container(out, GuFile, out);
	errno = 0;
	int ret = fflush(file->file);
	if (ret == EOF) {
		gu_raise_errno(err);
	}
}


GuFile*
gu_file(FILE* file, GuPool* pool)
{
	return gu_new_s(
		pool, GuFile,
		.file = file,
		.in = { gu_file_input },
		.out = {
			.output = gu_file_output,
			.flush = gu_file_flush	 
				 });
}

