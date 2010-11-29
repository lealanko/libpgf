
#include "type.h"

GuKind gu_type__top = { NULL };

GuKind gu_type__named = { &gu_type__top };

GuKind gu_type__struct = { &gu_type__named };

GuKind gu_type_GuVariant = { &gu_type__named };

GU_DEFINE_TYPE(int, _named);



bool gu_type_has_kind(GuType* type, GuKind* kind)
{
	GuKind* k = &type->kind;
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
	GuKind* kind = &type->kind;
	while (kind != NULL) {
		void* val = gu_map_get(tmap, kind);
		if (val != NULL) {
			return val;
		}
		kind = kind->super;
	}
	return NULL;
}

