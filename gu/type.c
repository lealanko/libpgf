
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

GU_DEFINE_KIND(GuVariant, repr);



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

