#include "variant.h"

enum {
	ALIGNMENT = sizeof(guintptr)
};

gpointer 
gu_variant_alloc(GuAllocator* ator, guint8 tag, gsize size, 
		 gsize align, GuVariant* variant_out)
{
	if (align == 0) {
		align = gu_mem_alignment(size);
	}
	align = MAX(ALIGNMENT, align);
	if (((gsize)tag) > ALIGNMENT - 2) {
		guint8* alloc = gu_malloc_aligned(ator, align + size, align);
		alloc[align - 1] = tag;
		gpointer p = &alloc[align];
		variant_out->p = (guintptr)p;
		return p;
	}
	gpointer p = gu_malloc_aligned(ator, size, align);
	variant_out->p = ((guintptr)p) | (tag + 1);
	return p;
}

guint
gu_variant_tag(GuVariant variant)
{
	guint u = variant.p % ALIGNMENT;
	if (u == 0) {
		guint8* mem = (guint8*)variant.p;
		return mem[-1];
	}
	return u - 1;
}

gpointer
gu_variant_data(GuVariant variant)
{
	return (gpointer)((variant.p / ALIGNMENT) * ALIGNMENT);
}

gint 
gu_variant_intval(GuVariant variant)
{
	guint u = variant.p % ALIGNMENT;
	if (u == 0) {
		gint* mem = (gint*)variant.p;
		return *mem;
	}
	return (variant.p / ALIGNMENT);
}


struct foo {
	gint l;
	struct {
		gint u;
		gchar arr[];
	} fin;
};
