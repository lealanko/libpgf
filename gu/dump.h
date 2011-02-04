#ifndef GU_DUMP_H_
#define GU_DUMP_H_

#include <gu/defs.h>
#include <gu/yaml.h>
#include <gu/type.h>
#include <gu/map.h>

typedef struct GuDumpCtx GuDumpCtx;

struct GuDumpCtx {
	GuPool* pool;
	GuYaml* yaml;
	GuMap* data;
	GuTypeMap* dumpers;
};

typedef void (*GuDumpFn)(GuFn* self, GuType* type, const void* value, GuDumpCtx* ctx);

GuDumpCtx*
gu_dump_ctx_new(GuPool* pool, FILE* out, GuTypeTable* dumpers);

void
gu_dump(GuType* type, const void* value, GuDumpCtx* ctx);

extern GuTypeTable
gu_dump_table;


#endif // GU_DUMP_H_
