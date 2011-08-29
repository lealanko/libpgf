#include <gu/dump.h>
#include <gu/list.h>
#include <gu/variant.h>
#include <gu/seq.h>
#include <gu/assert.h>

GuDumpCtx*
gu_dump_ctx_new(GuPool* pool, FILE* out, GuTypeTable* dumpers) {
	GuDumpCtx* ctx = gu_new(pool, GuDumpCtx);
	ctx->pool = pool;
	if (dumpers == NULL) {
		dumpers = &gu_dump_table;
	}
	ctx->dumpers = gu_type_map_new(pool, dumpers);
	ctx->yaml = gu_yaml_new(pool, out);
	ctx->data = gu_map_new(pool, NULL, NULL);
	ctx->print_address = false;
	return ctx;
}

void
gu_dump(GuType* type, const void* value, GuDumpCtx* ctx)
{
	GuDumpFn* dumper = gu_type_map_get(ctx->dumpers, type);
	if (ctx->print_address) {
		GuString* str = gu_string_format(ctx->pool, "%p", value);
		gu_yaml_comment(ctx->yaml, str);
	}
	(*dumper)(dumper, type, value, ctx);
}

void
gu_dump_stderr(GuType* type, const void* value)
{
	GuPool* pool = gu_pool_new();
	GuDumpCtx* ctx = gu_dump_ctx_new(pool, stderr, NULL);
	gu_dump(type, value, ctx);
	gu_pool_free(pool);
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
gu_dump_uint16(GuDumpFn* dumper, GuType* type, const void* p, 
	       GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	const uint16_t* ip = p;
	GuString* str = gu_string_format(ctx->pool, "%" PRIu16, *ip);
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
	if (*pp == NULL) {
		gu_yaml_tag_secondary(ctx->yaml, gu_cstring("null"));
		gu_yaml_scalar(ctx->yaml, gu_string_empty);
	} else {
		gu_dump(ptype->pointed_type, *pp, ctx);
	}
}

typedef struct {
	GuMapIterFn fn;
	GuMapType* mtype;
	GuDumpCtx* ctx;
} GuDumpMapFn;

static void
gu_dump_map_entry_fn(GuMapIterFn* self, const void* key, void* value)
{
	GuDumpMapFn* clo = (GuDumpMapFn*) self;
	gu_dump(clo->mtype->key_type, key, clo->ctx);
	gu_dump(clo->mtype->value_type, value, clo->ctx);
}

static void
gu_dump_map(GuDumpFn* dumper, GuType* type, const void* p,
	    GuDumpCtx* ctx)
{
	(void) dumper;
	GuMapType* mtype = (GuMapType*) type;
	GuMap* map = (GuMap*) p;
	gu_yaml_begin_mapping(ctx->yaml);
	GuDumpMapFn clo = { { gu_dump_map_entry_fn }, mtype, ctx };
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
			gu_assert(len >= 0);
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
gu_dump_alias(GuDumpFn* dumper, GuType* type, const void* p,
	      GuDumpCtx* ctx)
{
	(void) dumper;
	GuTypeAlias* alias = gu_type_cast(type, alias);

	gu_dump(alias->type, p, ctx);
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
	GuTypeAlias* alias = gu_type_cast(type, alias);
	bool created = gu_dump_anchor(ctx, p);
	if (created) {
		gu_dump(alias->type, p, ctx);
	} else {
		// gu_assert(false);
	}
}

static void 
gu_dump_reference(GuDumpFn* dumper, GuType* type, const void* p, 
		  GuDumpCtx* ctx)
{
	(void) dumper;
	(void) type;
	void* const* pp = p;
	bool created = gu_dump_anchor(ctx, *pp);
	if (created) {
		// gu_assert(false);
		GuPointerType* ptype = (GuPointerType*) type;
		gu_dump(ptype->pointed_type, *pp, ctx);
	}
}

static void 
gu_dump_shared(GuDumpFn* dumper, GuType* type, const void* p, 
		  GuDumpCtx* ctx)
{
	(void) dumper;
	void* const* pp = p;
	if (*pp == NULL) {
		gu_yaml_tag_secondary(ctx->yaml, gu_cstring("null"));
		gu_yaml_scalar(ctx->yaml, gu_string_empty);
	} else {
		bool created = gu_dump_anchor(ctx, *pp);
		if (created) {
			GuPointerType* ptype = (GuPointerType*) type;
			gu_dump(ptype->pointed_type, *pp, ctx);
		}
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
	size_t elem_size = gu_type_size(ltype->elem_type);
	gu_yaml_begin_sequence(ctx->yaml);
	for (int i = 0; i < len; i++) {
		ptrdiff_t offset = ltype->elems_offset + i * elem_size;
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

static void
gu_dump_seq(GuDumpFn* dumper, GuType* type, const void* p,
	    GuDumpCtx* ctx)
{
	(void) dumper;
	GuSeqType* dtype = gu_type_cast(type, GuSeq);
	size_t elem_size = gu_type_size(dtype->elem_type);
	const GuSeq* seq = p;
	int len = gu_seq_size(seq);
	gu_yaml_begin_sequence(ctx->yaml);
	for (int i = 0; i < len; i += elem_size) {
		const void* elemp = gu_seq_index((GuSeq*) seq, i);
		gu_dump(dtype->elem_type, elemp, ctx);
	}
	gu_yaml_end(ctx->yaml);
}


GuTypeTable
gu_dump_table = GU_TYPETABLE(
	GU_SLIST_0,
	{ gu_kind(int), gu_fn(gu_dump_int) },
	{ gu_kind(uint16_t), gu_fn(gu_dump_uint16) },
	{ gu_kind(GuString), gu_fn(gu_dump_string) },
	{ gu_kind(struct), gu_fn(gu_dump_struct) },
	{ gu_kind(pointer), gu_fn(gu_dump_pointer) },
	{ gu_kind(GuMap), gu_fn(gu_dump_map) },
	{ gu_kind(alias), gu_fn(gu_dump_alias) },
	{ gu_kind(reference), gu_fn(gu_dump_reference) },
	{ gu_kind(referenced), gu_fn(gu_dump_referenced) },
	{ gu_kind(shared), gu_fn(gu_dump_shared) },
	{ gu_kind(GuList), gu_fn(gu_dump_list) },
	{ gu_kind(GuSeq), gu_fn(gu_dump_seq) },
	{ gu_kind(GuLength), gu_fn(gu_dump_length) },
	{ gu_kind(GuVariant), gu_fn(gu_dump_variant) },
	{ gu_kind(double), gu_fn(gu_dump_double) },
	{ gu_kind(enum), gu_fn(gu_dump_enum) },
	);
