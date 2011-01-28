
#include "type.h"

GU_DEFINE_KIND(type, NULL);

GU_DEFINE_KIND(typedef, gu_kind(type));
GU_DEFINE_KIND(referenced, gu_kind(typedef));

GU_DEFINE_KIND(repr, gu_kind(type));

GU_DEFINE_KIND(abstract, gu_kind(type));

GU_DEFINE_KIND(struct, gu_kind(repr));

GU_DEFINE_KIND(pointer, gu_kind(repr));
GU_DEFINE_KIND(reference, gu_kind(pointer));
GU_DEFINE_KIND(shared, gu_kind(pointer));

GU_DEFINE_KIND(primitive, gu_kind(repr));
GU_DEFINE_KIND(integer, gu_kind(primitive));

GU_DEFINE_TYPE(int, integer, _);

GU_DEFINE_KIND(GuVariant, gu_kind(repr));



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

