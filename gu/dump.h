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
	bool print_address;
};

typedef void (*GuDumpFn)(GuFn* self, GuType* type, const void* value, GuDumpCtx* ctx);

GuDumpCtx*
gu_dump_ctx_new(GuPool* pool, GuWriter* wtr, GuTypeTable* dumpers);

void
gu_dump(GuType* type, const void* value, GuDumpCtx* ctx);

void
gu_dump_stderr(GuType* type, const void* value);

extern GuTypeTable
gu_dump_table;


#endif // GU_DUMP_H_
