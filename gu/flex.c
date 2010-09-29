#include "flex.h"

#include <string.h>

gpointer gu_list_alloc(GuAllocator* ator, gsize base_size, gsize elem_size, 
		       gint n_elems, gsize alignment, goffset len_offset)
{
	g_assert(n_elems >= 0);
	gpointer p = gu_malloc_aligned(ator, base_size + elem_size * n_elems,
				       alignment);
	G_STRUCT_MEMBER(gint, p, len_offset) = n_elems;
	return p;
}



guint gu_bytes_hash(const GuBytes* bytes)
{
	// Modified from g_string_hash in glib
	gint n = gu_list_length(bytes);
	const guint8* p = gu_list_elems(bytes);
	guint h = 0;

	while (n--) {
		h = (h << 5) - h + *p;
		p++;
	}

	return h;
}

gboolean gu_bytes_equal(const GuBytes* b1, const GuBytes* b2)
{
	gint len1 = gu_list_length(b1);
	gint len2 = gu_list_length(b2);
	if (len1 != len2) {
		return FALSE;
	}
	const guint8* data1 = gu_list_elems(b1);
	const guint8* data2 = gu_list_elems(b2);
	gint cmp = memcmp(data1, data2, len1);
	return (cmp == 0);
}
