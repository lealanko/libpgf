#ifndef GU_FILE_H_
#define GU_FILE_H_

#include <gu/in.h>
#include <gu/out.h>
#include <gu/write.h>
#include <gu/read.h>
#include <stdio.h>

typedef const struct GuFile GuFile;

struct GuFile
{
	FILE* file;
	GuWriter wtr;
	GuReader rdr;
	GuIn in;
	GuOut out;
};

GuFile*
gu_file(FILE* file, GuPool* pool);


#endif // GU_FILE_H_
