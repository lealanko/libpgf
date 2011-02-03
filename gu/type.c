
#include "type.h"

GuKind gu_type_type = { .super = NULL };

GU_DEFINE_KIND(typedef, type);
GU_DEFINE_KIND(referenced, typedef);

GU_DEFINE_KIND(repr, type);

GU_DEFINE_KIND(abstract, type);

GU_DEFINE_KIND(struct, repr);

GU_DEFINE_KIND(pointer, repr);
GU_DEFINE_KIND(reference, pointer);
GU_DEFINE_KIND(shared, pointer);

GU_DEFINE_KIND(primitive, repr);
GU_DEFINE_KIND(integer, primitive);

GU_DEFINE_TYPE(int, integer, _);

GU_DEFINE_TYPE(GuLength, int, _);

GU_DEFINE_TYPE(float, primitive, _);
GU_DEFINE_TYPE(double, primitive, _);


GU_DEFINE_KIND(enum, repr);

bool gu_type_has_kind(GuType* type, GuKind* kind)
{
	GuKind* k = (GuKind*)type;
	while (k != NULL) {
		if (k == kind) {
			return true;
		}
		k = k->super;
	}
	return false;
}


static void gu_type_map_init(GuTypeMap* tmap, GuTypeTable* table) 
{
	for (int i = 0; i < table->parents.len; i++) {
		gu_type_map_init(tmap, table->parents.elems[i]);
	}
	for (int i = 0; i < table->entries.len; i++) {
		GuTypeTableEntry* e = &table->entries.elems[i];
		gu_map_set(tmap, e->kind, e->val);
	}
}

GuTypeMap* gu_type_map_new(GuPool* pool, GuTypeTable* table)
{
	GuTypeMap* tmap = gu_map_new(pool, NULL, NULL);
	gu_type_map_init(tmap, table);
	return tmap;
}

void* gu_type_map_lookup(GuTypeMap* tmap, GuType* type)
{
	GuKind* kind = (GuKind*)type;
	while (kind != NULL) {
		void* val = gu_map_get(tmap, kind);
		if (val != NULL) {
			return val;
		}
		kind = kind->super;
	}
	return NULL;
}


size_t gu_type_size(GuType* type) {
	if (gu_type_has_kind(type, gu_kind(repr))) {
		GuType_repr* repr = (GuType_repr*) type;
		return repr->size;
	} else if (gu_type_has_kind(type, gu_kind(typedef))) {
		GuTypeDef* tdef = (GuTypeDef*) type;
		return gu_type_size(tdef->type);
	} else {
		return 0;
	}
}

const void* 
gu_type_check_cast(GuType* type, GuKind* kind)
{
	g_assert(gu_type_has_kind(type, kind));
	return type;
}

GuEnumConstant*
gu_enum_value(GuEnumType* etype, const void* enump)
{
	size_t esize = etype->repr_base.size;
#define CHECK_ENUM_TYPE(t_) do {					\
		if (esize == sizeof(t_)) {				\
			t_ c = *(const t_*)enump;			\
			for (int i = 0; i < etype->constants.len; i++) { \
				GuEnumConstant* cp = &etype->constants.elems[i]; \
				t_ d = *(const t_*)cp->enum_value;	\
				if (c == d) {				\
					return cp;			\
				}					\
			}						\
			return NULL;					\
		}							\
	} while (false)

	CHECK_ENUM_TYPE(int);
	CHECK_ENUM_TYPE(char);
	CHECK_ENUM_TYPE(short);
	CHECK_ENUM_TYPE(long);
	CHECK_ENUM_TYPE(long long);

	return NULL;
}
