#ifndef GU_FILE_H_
#define GU_FILE_H_

#include <gu/in.h>
#include <gu/out.h>
#include <stdio.h>

typedef const struct GuFile GuFile;

struct GuFile
{
	FILE* file;
	GuIn in;
	GuOut out;
};

GuFile*
gu_file(FILE* file, GuPool* pool);


#endif // GU_FILE_H_
