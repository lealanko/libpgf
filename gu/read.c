#include <gu/read.h>


size_t
gu_read(GuReader* rdr, wchar_t* buf, size_t len, GuError* err)
{
	return rdr->read(rdr, buf, len, err);
}

wint_t
gu_getc(GuReader* rdr, GuError* err)
{
	wchar_t wc = L'\0';
	size_t got = gu_read(rdr, &wc, 1, err);
	if (got < 1) {
		return WEOF;
	}
	return wc;
}

typedef struct GuWcsReader GuWcsReader;

struct GuWcsReader {
	GuReader rdr;
	const wchar_t* wcs;
	size_t size;
	size_t idx;
};

static size_t
gu_wcs_reader_read(GuReader* rdr, wchar_t* buf, size_t len, GuError* err)
{
	(void) err;
	GuWcsReader* wr = (GuWcsReader*) rdr;
	len = GU_MIN(len, wr->size - wr->idx);
	wmemcpy(buf, &wr->wcs[wr->idx], len);
	wr->idx += len;
	return len;
}

GuReader*
gu_wcs_reader(const wchar_t* wcs, size_t len, GuPool* pool)
{
	return (GuReader*) 
		gu_new_s(pool, GuWcsReader, 
			 .rdr = { .read = gu_wcs_reader_read },
			 .wcs = wcs,
			 .size = len,
			 .idx = 0);
}
