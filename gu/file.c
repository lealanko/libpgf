#include <gu/file.h>


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
		.out = {
			.output = gu_file_output,
			.flush = gu_file_flush	 
				 });
}


typedef struct GuFileInStream GuFileInStream;

struct GuFileInStream {
	GuInStream stream;
	FILE* file;
};

static size_t 
gu_file_input(GuInStream* stream, uint8_t* buf, size_t sz, GuError* err)
{
	GuFileInStream* fis = gu_container(stream, GuFileInStream, stream);
	errno = 0;
	size_t got = fread(buf, 1, sz, fis->file);
	if (got == 0) {
		if (ferror(fis->file)) {
			gu_raise_errno(err);
		}
	}
	return got;
}

GuIn*
gu_file_in(FILE* file, GuPool* pool)
{
	GuFileInStream* fis = gu_new_s(pool, GuFileInStream,
				       .stream.input = gu_file_input,
				       .file = file);
	return gu_make_in(&fis->stream, pool);
}
