#ifndef GU_FILE_H_
#define GU_FILE_H_

#include <gu/in.h>
#include <gu/out.h>
#include <stdio.h>

typedef struct GuFile GuFile;

struct GuFile
{
	FILE* file;
	GuOut out;
};

GuFile*
gu_file(FILE* file, GuPool* pool);

GuIn* 
gu_file_in(FILE* file, GuPool* pool);

#endif // GU_FILE_H_
