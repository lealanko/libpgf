#ifndef GU_MACROS_H_
#define GU_MACROS_H_

#include <glib.h>
#include <guconfig.h>

#define GU_CONTAINER_P(mem_p, container_type, member) \
	((container_type*)(((guint8*) (mem_p)) - G_STRUCT_OFFSET(container_type, member)))

#ifndef gu_alignof
#define gu_alignof(type) \
	((gsize)(offsetof(struct { char _c; type _t; }, _t)))
#ifdef GU_CAN_HAVE_FAM_IN_MEMBER
#define GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#endif
#else
#define GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#endif


#define GU_PTRLIT(type, expr) \
	((type[1]){ expr })

#define GU_LVALUE(type, expr) \
	(*((type[1]){ expr }))

#define GU_COMMA ,


#endif // GU_MACROS_H_
