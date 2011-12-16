#include <gu/read.h>

extern inline GuUCS
gu_read_ucs(GuReader* rdr, GuError* err);

extern inline char
gu_getc(GuReader* rdr, GuError* err);

GuReader*
gu_make_utf8_reader(GuIn* utf8_in, GuPool* pool)
{
	
}
