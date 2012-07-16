// Copyright 2011 University of Helsinki. Released under LGPL3.

#include <gu/file.h>

typedef struct GuFileOutStream GuFileOutStream;

struct GuFileOutStream {
	GuOutStream stream;
	FILE* file;
};

static size_t
gu_file_output(GuOutStream* stream, GuCSlice src, GuExn* err)
{
	GuFileOutStream* fos = gu_container(stream, GuFileOutStream, stream);
	errno = 0;
	size_t wrote = fwrite(src.p, 1, src.sz, fos->file);
	if (wrote < src.sz) {
		if (ferror(fos->file)) {
			gu_raise_errno(err);
		}
	}
	return wrote;
}

static void
gu_file_flush(GuOutStream* stream, GuExn* err)
{
	GuFileOutStream* fos = gu_container(stream, GuFileOutStream, stream);
	errno = 0;
	if (fflush(fos->file) != 0) {
		gu_raise_errno(err);
	}
}

GuOut*
gu_file_out(FILE* file, GuPool* pool)
{
	GuFileOutStream* fos = gu_new_i(pool, GuFileOutStream,
					.stream.output = gu_file_output,
					.stream.flush = gu_file_flush,
					.file = file);
	return gu_new_out(&fos->stream, pool);
}


typedef struct GuFileInStream GuFileInStream;

struct GuFileInStream {
	GuInStream stream;
	FILE* file;
};

static size_t 
gu_file_input(GuInStream* stream, GuSlice buf, GuExn* err)
{
	GuFileInStream* fis = gu_container(stream, GuFileInStream, stream);
	errno = 0;
	size_t got = fread(buf.p, 1, buf.sz, fis->file);
	if (got < buf.sz) {
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
	return gu_new_in(&fis->stream, pool);
}
