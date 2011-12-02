#ifndef GU_UTF8_H_
#define GU_UTF8_H_

#include <gu/in.h>
#include <gu/out.h>
#include <gu/read.h>
#include <gu/write.h>

GuUCS
gu_in_utf8(GuIn* in, GuError* err);

void
gu_out_utf8(GuUCS uc, GuOut* out, GuError* err);

void
gu_utf8_write(const uint8_t* buf, size_t len, GuWriter* wtr, GuError* err);

GuWriter*
gu_utf8_writer(GuOut* out, GuPool* pool);

GuReader*
gu_utf8_reader(GuIn* in, GuPool* pool);


#endif // GU_UTF8_H_
