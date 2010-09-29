#ifndef GU_VARIANT_H_
#define GU_VARIANT_H_

#include "mem.h"

typedef struct GuVariant GuVariant;

gpointer gu_variant_alloc(GuAllocator* ator, guint8 tag, 
			  gsize size, gsize align, 
			  GuVariant* variant_out);

#define gu_variant_new(ator, tag, type, variant_out) \
	((type*)gu_variant_alloc(ator, tag, sizeof(type), variant_out))

#define gu_variant_flex_new(ator, tag, type, flex_mem, n_elems, variant_out)	\
	((type*)gu_variant_alloc(ator, tag,				\
				 GU_FLEX_SIZE(type, flex_mem, n_elems), \
				 variant_out)

guint gu_variant_tag(GuVariant variant);

gpointer gu_variant_data(GuVariant variant);


// <private>

struct GuVariant {
	guintptr p;
};

#endif // GU_VARIANT_H_
