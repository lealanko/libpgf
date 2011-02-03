/* 
 * Copyright 2010 University of Helsinki.
 *   
 * This file is part of libgu.
 * 
 * Libgu is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * Libgu is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libgu. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *
 * Miscellaneous macros.
 */

#ifndef GU_DEFS_H_
#define GU_DEFS_H_

#include <guconfig.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <glib.h>

#define GU_CONTAINER_P(mem_p, container_type, member) \
	((container_type*)(((uint8_t*) (mem_p)) - offsetof(container_type, member)))
/**< Find the address of a containing structure.
 *
 * If @c s has type @c t*, where @c t is a struct or union type with a
 * member @m, then <tt>GU_CONTAINER_P(&s->m, t, m) == s</tt>.
 * 
 * @param mem_p           Pointer to the member of a structure.
 * @param container_type  The type of the containing structure.
 * @param member          The name of the member of @a container_type
 * @return  The address of the containing structure.
 *
 * @hideinitializer */

#define gu_container(mem_p, container_type, member) \
	GU_CONTAINER_P(mem_p, container_type, member)


#ifndef gu_alignof
#define gu_alignof(type) \
	((size_t)(offsetof(struct { char _c; type _t; }, _t)))
#ifdef GU_CAN_HAVE_FAM_IN_MEMBER
#define GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#endif
#else
#define GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#endif

#define GU_PLIT(type, expr) \
	((type[1]){ expr })

#define GU_LVALUE(type, expr) \
	(*((type[1]){ expr }))

#define GU_COMMA ,

#define GU_ARRAYLIT_LEN(t,a) (sizeof((const t[])a) / sizeof(t))

#define GU_ARG1(a1, ...) a1
#define GU_ARG2(a1, a2, ...) a2

#define GU_BEGIN do {
#define GU_END } while (false)

#ifdef GU_ALIGNOF_WORKS_ON_FAM_STRUCTS
#define gu_flex_alignof gu_alignof
#else
#define gu_flex_alignof(t) 0
#endif

#define GU_FLEX_SIZE(type, flex_member, n_elems) \
	(sizeof(type) + ((n_elems) * sizeof(((type *)NULL)->flex_member[0])))
/**< @hideinitializer */

#define gu_assert(expr) g_assert(expr)


#endif // GU_DEFS_H_
