#include <gu/read.h>

extern inline GuUCS
gu_read_ucs(GuReader* rdr, GuExn* err);

extern inline char
gu_getc(GuReader* rdr, GuExn* err);

GuReader*
gu_new_utf8_reader(GuIn* utf8_in, GuPool* pool)
{
	
}
