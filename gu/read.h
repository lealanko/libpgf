#ifndef GU_READ_H_
#define GU_READ_H_

#include <gu/mem.h>
#include <gu/error.h>
#include <wchar.h>

typedef const struct GuReader GuReader;

struct GuReader {
	size_t (*read)(GuReader* rdr, wchar_t* buf, size_t len, GuError* err);
};

size_t
gu_read(GuReader* rdr, wchar_t* buf, size_t len, GuError* err);

wint_t
gu_getc(GuReader* rdr, GuError* err);

GuReader*
gu_wcs_reader(const wchar_t* wcs, size_t len, GuPool* pool);

#endif // GU_READ_H_
