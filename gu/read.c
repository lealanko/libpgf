#include <gu/read.h>


size_t
gu_read(GuReader* rdr, GuUCS* buf, size_t max_len, GuError* err)
{
	return rdr->read(rdr, buf, max_len, err);
}

size_t
gu_read_all(GuReader* rdr, GuUCS* buf, size_t len, GuError* err)
{
	size_t have = 0;
	while (have < len && gu_ok(err)) {
		size_t got = gu_read(rdr, &buf[have], len - have, err);
		if (got == 0) {
			break;
		}
		have += got;
	}
	return have;
}


GuUCS
gu_read_ucs(GuReader* rdr, GuError* err)
{
	GuUCS ucs = 0;
	size_t got = gu_read_all(rdr, &ucs, 1, err);
	if (got < 1) {
		gu_raise(err, GuEOF);
		return 0;
	}
	return ucs;
}

char
gu_getc(GuReader* rdr, GuError* err)
{
	return gu_ucs_char(gu_read_ucs(rdr, err), err);
}

typedef struct GuCharReader GuCharReader;

struct GuCharReader {
	GuReader rdr;
	GuIn* in;
};

static size_t
gu_char_reader_read(GuReader* rdr, GuUCS* buf, size_t max_len, GuError* err)
{
	GuCharReader* crdr = (GuCharReader*) rdr;
	size_t have = 0;
	while (have < max_len && gu_ok(err)) {
		uint8_t cbuf[256];
		size_t req = GU_MIN(256, max_len - have);
		size_t got = gu_in_some(crdr->in, cbuf, req, err);
		gu_error_block(err);
		size_t n = gu_str_to_ucs((char*) cbuf, got, &buf[have], err);
		gu_error_unblock(err);
		have += n;
		if (n < req) {
			break;
		}
	}
	return have;
}

GuReader*
gu_char_reader(GuIn* in, GuPool* pool)
{
	return (GuReader*) gu_new_s(pool, GuCharReader,
				    .rdr = { gu_char_reader_read },
				    .in = in);
}
