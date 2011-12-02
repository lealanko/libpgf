#ifndef GU_READ_H_
#define GU_READ_H_

#include <gu/ucs.h>

typedef const struct GuReader GuReader;

struct GuReader {
	size_t (*read)(GuReader* rdr, GuUCS* buf, size_t max_len, GuError* err);
};

size_t
gu_read(GuReader* rdr, GuUCS* buf, size_t max_len, GuError* err);

size_t
gu_read_all(GuReader* rdr, GuUCS* buf, size_t len, GuError* err);

GuUCS
gu_read_ucs(GuReader* rdr, GuError* err);

char
gu_getc(GuReader* rdr, GuError* err);

#include <gu/in.h>

GuReader*
gu_char_reader(GuIn* in, GuPool* pool);

#endif // GU_READ_H_
