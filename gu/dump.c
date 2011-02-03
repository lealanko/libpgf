#include <gu/dump.h>
#include <gu/list.h>
#include <gu/variant.h>

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
gu_dump_double(GuDumpFn* dumper, GuType* type, const void* p, 
	       GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	const double* dp = p;
	GuString* str = gu_string_format(ctx->pool, "%lf", *dp);
	gu_yaml_scalar(ctx->yaml, str);
}

static GU_DEFINE_ATOM(gu_dump_length_key, "gu_dump_length_key");

static void 
gu_dump_length(GuDumpFn* dumper, GuType* type, const void* p, 
	       GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	const GuLength* ip = p;
	GuString* str = gu_string_format(ctx->pool, "%d", *ip);
	gu_yaml_scalar(ctx->yaml, str);
	GuLength* lenp = gu_map_get(ctx->data, gu_atom(gu_dump_length_key));
	if (lenp != NULL) {
		*lenp = *ip;
	}
}



static void 
gu_dump_direct_int(GuDumpFn* dumper, GuType* type, const void* p, 
		   GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	void* const *pp = p;
	void *vp = *pp;
	int i = GPOINTER_TO_INT(vp);
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
	gu_dump(mtype->key_type, &key, ctx);
	gu_dump(mtype->value_type, &value, ctx);
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
	GuLength* old_lenp = 
		gu_map_get(ctx->data, gu_atom(gu_dump_length_key));
	GuLength len = -1;
	gu_map_set(ctx->data, gu_atom(gu_dump_length_key), &len);

	for (int i = 0; i < srepr->members.len; i++) {
		const GuMember* member = &srepr->members.elems[i];
		gu_yaml_scalar(ctx->yaml, member->name);
		const uint8_t* memp = &data[member->offset];
		if (member->is_flex) {
			// Flexible array member
			g_assert(len >= 0);
			size_t mem_s = gu_type_size(member->type);
			gu_yaml_begin_sequence(ctx->yaml);
			for (int i = 0; i < len; i++) {
				gu_dump(member->type, &memp[i * mem_s], ctx);
			}
			gu_yaml_end(ctx->yaml);
		} else {
			gu_dump(member->type, memp, ctx);
		}
	}
	gu_yaml_end(ctx->yaml);
	gu_map_set(ctx->data, gu_atom(gu_dump_length_key), old_lenp);
}

static void
gu_dump_typedef(GuDumpFn* dumper, GuType* type, const void* p,
		GuDumpCtx* ctx)
{
	(void) dumper;
	GuTypeDef* tdef = (GuTypeDef*) type;
	gu_dump(tdef->type, p, ctx);
}

static const char gu_dump_reference_key[] = "reference";

static bool
gu_dump_anchor(GuDumpCtx* ctx, const void* p) 
{
	GuMap* map = gu_map_get(ctx->data, gu_dump_reference_key);
	if (map == NULL) {
		map = gu_map_new(ctx->pool, NULL, NULL);
		gu_map_set(ctx->data, gu_dump_reference_key, map);
	}
	GuYamlAnchor* ap = gu_map_get(map, p);
	if (ap == NULL) {
		ap = gu_new(ctx->pool, GuYamlAnchor);
		*ap = gu_yaml_anchor(ctx->yaml);
		gu_map_set(map, p, ap);
		return true;
	} else {
		gu_yaml_alias(ctx->yaml, *ap);
		return false;
	}
}

static void
gu_dump_referenced(GuDumpFn* dumper, GuType* type, const void* p,
		   GuDumpCtx* ctx)
{
	(void) dumper;
	GuTypeDef* tdef = (GuTypeDef*) type;
	bool created = gu_dump_anchor(ctx, p);
	g_assert(created);
	gu_dump(tdef->type, p, ctx);
}

static void 
gu_dump_reference(GuDumpFn* dumper, GuType* type, const void* p, 
		  GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	void* const* pp = p;
	bool created = gu_dump_anchor(ctx, *pp);
	g_assert(!created);
}

static void 
gu_dump_shared(GuDumpFn* dumper, GuType* type, const void* p, 
		  GuDumpCtx* ctx)
{
	(void) dumper;
	void* const* pp = p;
	bool created = gu_dump_anchor(ctx, *pp);
	if (created) {
		GuPointerType* ptype = (GuPointerType*) type;
		gu_dump(ptype->pointed_type, *pp, ctx);
	}
}

static void 
gu_dump_list(GuDumpFn* dumper, GuType* type, const void* p, 
	     GuDumpCtx* ctx)
{
	(void) dumper;
	GuListType* ltype = (GuListType*) type;
	const uint8_t* up = p;
	int len = * (const int*) p;
	gu_yaml_begin_sequence(ctx->yaml);
	for (int i = 0; i < len; i++) {
		ptrdiff_t offset = ltype->elems_offset + i * ltype->elem_size;
		gu_dump(ltype->elem_type, &up[offset], ctx);
	}
	gu_yaml_end(ctx->yaml);
}

static void
gu_dump_variant(GuDumpFn* dumper, GuType* type, const void* p,
		GuDumpCtx* ctx)
{
	(void) dumper;
	GuVariantType* vtype = gu_type_cast(type, GuVariant);
	const GuVariant* vp = p;
	int tag = gu_variant_tag(*vp);
	for (int i = 0; i < vtype->ctors.len; i++) {
		GuConstructor* ctor = &vtype->ctors.elems[i];
		if (ctor->c_tag == tag) {
			gu_yaml_begin_mapping(ctx->yaml);
			gu_yaml_scalar(ctx->yaml, ctor->c_name);
			void* data = gu_variant_data(*vp);
			gu_dump(ctor->type, data, ctx);
			gu_yaml_end(ctx->yaml);
			return;
		}
	}
	gu_assert(false);
}

static void
gu_dump_enum(GuDumpFn* dumper, GuType* type, const void* p,
	     GuDumpCtx* ctx)
{
	(void) dumper;
	GuEnumType* etype = gu_type_cast(type, enum);
	GuEnumConstant* cp = gu_enum_value(etype, p);
	gu_assert(cp != NULL);
	gu_yaml_scalar(ctx->yaml, cp->name);
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
	{ gu_kind(reference), gu_fn(gu_dump_reference) },
	{ gu_kind(referenced), gu_fn(gu_dump_referenced) },
	{ gu_kind(shared), gu_fn(gu_dump_shared) },
	{ gu_kind(GuList), gu_fn(gu_dump_list) },
	{ gu_kind(GuLength), gu_fn(gu_dump_length) },
	{ gu_kind(GuVariant), gu_fn(gu_dump_variant) },
	{ gu_kind(double), gu_fn(gu_dump_double) },
	{ gu_kind(enum), gu_fn(gu_dump_enum) },
	);
