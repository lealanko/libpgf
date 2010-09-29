#ifndef GU_MEM_H_
#define GU_MEM_H_

#include <glib.h>
#include "macros.h"

G_BEGIN_DECLS


typedef struct GuAllocator GuAllocator;

gpointer gu_malloc_aligned(GuAllocator* ator, gsize size, gsize alignment);

inline gpointer
gu_malloc(GuAllocator* ator, gsize size) {
	return gu_malloc_aligned(ator, size, 0);
}

#define gu_new(ator, type) \
	((type*)gu_malloc_aligned((ator), sizeof(type), gu_alignof(type)))

GuAllocator* gu_slice_allocator(void);
GuAllocator* gu_malloc_allocator(void);

typedef struct GuMemPool GuMemPool;

GuMemPool* gu_mem_pool_new(void);

GuAllocator* gu_mem_pool_allocator(GuMemPool* pool);

void gu_mem_pool_register_finalizer(GuMemPool* pool, 
				    GDestroyNotify func, gpointer p);

void gu_mem_pool_free(GuMemPool* pool);

gsize gu_mem_alignment(gsize size);

G_END_DECLS

#endif // GU_MEM_H_
