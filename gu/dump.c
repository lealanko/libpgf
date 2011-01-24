#include "dump.h"
#include "flex.h"

void
gu_dump(GuType* type, const void* value, GuDumpCtx* ctx)
{
	GuDumpFn* dumper = gu_type_map_lookup(ctx->dumpers, type);
	(*dumper)(dumper, type, value, ctx);
}

static void 
gu_dump_int(GuDumpFn* dumper, GuType* type, const void* p, 
	    GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	const int* ip = p;
	GuString* str = gu_string_format(ctx->pool, "%d", *ip);
	gu_yaml_scalar(ctx->yaml, str);
}

static void 
gu_dump_direct_int(GuDumpFn* dumper, GuType* type, const void* p, 
		   GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	int i = GPOINTER_TO_INT(p);
	GuString* str = gu_string_format(ctx->pool, "%d", i);
	gu_yaml_scalar(ctx->yaml, str);
}

static void 
gu_dump_string(GuDumpFn* dumper, GuType* type, const void* p, 
	       GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	const GuString* s = p;
	gu_yaml_scalar(ctx->yaml, s);
}


// For _non-shared_ pointers.
static void 
gu_dump_pointer(GuDumpFn* dumper, GuType* type, const void* p, 
		GuDumpCtx* ctx)
{
	(void) dumper;
	GuPointerType* ptype = (GuPointerType*) type;
	void* const* pp = p;
	gu_dump(ptype->pointed_type, *pp, ctx);
}

static void
gu_dump_map_entry_fn(GuFn* fnp, void* key, void* value)
{
	GuClo2* clo = (GuClo2*) fnp;
	GuMapType* mtype = clo->env1;
	GuDumpCtx* ctx = clo->env2;
	gu_dump(mtype->key_type, key, ctx);
	gu_dump(mtype->value_type, value, ctx);
}


static void
gu_dump_map(GuDumpFn* dumper, GuType* type, const void* p,
	    GuDumpCtx* ctx)
{
	(void) dumper;
	GuMapType* mtype = (GuMapType*) type;
	GuMap* map = (GuMap*) p;
	gu_yaml_begin_mapping(ctx->yaml);
	GuClo2 clo = { gu_dump_map_entry_fn, mtype, ctx };
	gu_map_iter(map, &clo.fn);
	gu_yaml_end(ctx->yaml);
}

static void
gu_dump_struct(GuDumpFn* dumper, GuType* type, const void* p,
	       GuDumpCtx* ctx)
{
	(void) dumper;
	GuStructRepr* srepr = (GuStructRepr*) type;
	gu_yaml_begin_mapping(ctx->yaml);
	const uint8_t* data = p;
	for (int i = 0; i < srepr->members.len; i++) {
		const GuMember* member = &srepr->members.elems[i];
		gu_yaml_scalar(ctx->yaml, member->name);
		const void* memp = &data[member->offset];
		gu_dump(member->type, memp, ctx);
	}
	gu_yaml_end(ctx->yaml);
}

static void
gu_dump_typedef(GuDumpFn* dumper, GuType* type, const void* p,
		GuDumpCtx* ctx)
{
	(void) dumper;
	GuTypeDef* tdef = (GuTypeDef*) type;
	gu_dump(tdef->type, p, ctx);
}

GuTypeTable
gu_dump_table = GU_TYPETABLE(
	GU_SLIST_0,
	{ gu_kind(int), gu_fn(gu_dump_int) },
	{ gu_kind(GuMapDirectInt), gu_fn(gu_dump_direct_int) },
	{ gu_kind(GuString), gu_fn(gu_dump_string) },
	{ gu_kind(struct), gu_fn(gu_dump_struct) },
	{ gu_kind(pointer), gu_fn(gu_dump_pointer) },
	{ gu_kind(GuMap), gu_fn(gu_dump_map) },
	{ gu_kind(typedef), gu_fn(gu_dump_typedef) },
	);
