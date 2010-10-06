/* 
 * Copyright 2010 University of Helsinki.
 *   
 * This file is part of libpgf.
 * 
 * Libpgf is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * Libpgf is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libpgf. If not, see <http://www.gnu.org/licenses/>.
 */

#include "data.h"
#include <gu/macros.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <glib/gprintf.h>

typedef struct PgfIdContext PgfIdContext;

struct PgfIdContext {
	gint count;
	gconstpointer current;
};

typedef struct PgfContext PgfContext;

struct PgfContext {
	PgfIdContext sequences;
	PgfIdContext cncfuns;
};

//
// PgfWriter
//

typedef struct PgfWriter PgfWriter;

struct PgfWriter {
	FILE* out;
	GError* err;
	PgfContext ctx;
};

static GQuark
pgf_writer_quark(void)
{
	return g_quark_from_static_string("pgf-writer-quark");
}


static void 
pgf_debugv(const gchar* fmt, va_list args)
{
#ifdef PGF_DEBUG
  g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, fmt, args);
#else
  (void) fmt;
  (void) args;
#endif
}



static void 
pgf_debug(const gchar* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  pgf_debugv(fmt, args);
  va_end (args);
}

static void
pgf_writer_error(PgfWriter* wtr, gint code, const gchar* format, ...)
{
	g_return_if_fail(wtr->err == NULL);
	va_list args;
	va_start(args, format);
	wtr->err = g_error_new_valist(pgf_writer_quark(), code, format, args);
	va_end(args);
	pgf_debug("Write error: %s", wtr->err->message);
}

static void
pgf_write_str(PgfWriter* wtr, const gchar* str)
{
	if (wtr->err != NULL) {
		return;
	}
	gint ret = fputs(str, wtr->out);
	if (ret < 0) {
		pgf_writer_error(wtr, 0, "Unable to write to output stream");
	}
}


static void
pgf_write_string(PgfWriter* wtr, const GuString* s)
{
	if (wtr->err != NULL) {
		return;
	}
	static gchar pgf_write_esc_exceptions[97] = { 0 };
	// XXX: not threadsafe
	if (pgf_write_esc_exceptions[0] == '\0') {
		for (gint i = 0; i < 96; i++) {
			pgf_write_esc_exceptions[i] = 128+i;
		}
	}
	gchar* str = g_strndup(s->elems, s->len);
	gint i = 0;
	pgf_write_str(wtr, "\"");
	// Make sure we print out NULs that the string could in theory contain
	do {
		gchar* esc = g_strescape(&str[i], pgf_write_esc_exceptions);
		pgf_write_str(wtr, esc);
		g_free(esc);
		i = i + strlen(str);
		for (i += strlen(str); str[i] == '\0' && i < s->len; i++) {
			pgf_write_str(wtr, "\\0");
		}
	} while (i < s->len);
	pgf_write_str(wtr, "\"");
}

static void
pgf_write_fmt(PgfWriter* wtr, const gchar* fmt, ...) G_GNUC_PRINTF (2, 3);

static void
pgf_write_fmt(PgfWriter* wtr, const gchar* fmt, ...)
{
	if (wtr->err != NULL) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	gint ret = g_vfprintf(wtr->out, fmt, args);
	va_end(args);
	if (ret < 0) {
		pgf_writer_error(wtr, 0, "Couldn't write to output stream");
	}
}

//
// PgfReader
// 

typedef struct PgfReader PgfReader;

struct PgfReader {
	FILE* in;
	GError* err;
	GuMemPool* pool;
	GuAllocator* ator;
	GHashTable* interned_strings;
	PgfContext ctx;
};

static PgfReader*
pgf_reader_new(FILE* in, GuMemPool* pool)
{
	PgfReader* rdr = gu_new(NULL, PgfReader);
	rdr->pool = pool;
	rdr->ator = gu_mem_pool_allocator(rdr->pool);
	rdr->interned_strings = 
		g_hash_table_new(gu_string_hash,
				 gu_string_equal);
	rdr->err = NULL;
	rdr->in = in;
	rdr->ctx = (PgfContext){{0, NULL}, {0, NULL}};
	return rdr;
}

static void
pgf_reader_free(PgfReader* rdr)
{
	g_hash_table_destroy(rdr->interned_strings);
	if (rdr->err != NULL) {
		g_error_free(rdr->err);
	}
	g_free(rdr);
}

static GQuark
pgf_reader_quark(void)
{
	return g_quark_from_static_string("pgf-reader-quark");
}

static void
pgf_reader_error(PgfReader* rdr, gint code, const gchar* format, ...)
{
	g_return_if_fail(rdr->err == NULL);
	va_list args;
	va_start(args, format);
	rdr->err = g_error_new_valist(pgf_reader_quark(), code, format, args);
	va_end(args);
	pgf_debug("Read error: %s", rdr->err->message);
}

static void
pgf_reader_tag_error(PgfReader* rdr, const char* type_name, gint tag)
{
	pgf_reader_error(rdr, 1, "Invalid tag for %s: %d", type_name, tag);
}

static gboolean
pgf_reader_failed(PgfReader* rdr)
{
	return (rdr->err != NULL);
}


static guint8 
pgf_read_u8(PgfReader* rdr) 
{
	if (pgf_reader_failed(rdr))
		return 0;
	int c = fgetc(rdr->in);
	if (c == EOF) {
		pgf_reader_error(rdr, 0, "Input stream error");
		return 0;
	}
	return (guint8)c;
}

static guint16
pgf_read_u16be(PgfReader* rdr)
{
	guint16 hi = pgf_read_u8(rdr);
	guint16 lo = pgf_read_u8(rdr);
	return hi << 8 | lo;
}

static guint32
pgf_read_u32be(PgfReader* rdr)
{
	guint32 hi = pgf_read_u16be(rdr);
	guint32 lo = pgf_read_u16be(rdr);
	return hi << 16 | lo;
}

static gint32
pgf_read_i32be(PgfReader* rdr)
{
	return (gint32)pgf_read_u32be(rdr);
}

static void
pgf_read_chars(PgfReader* rdr, gchar* buf, gsize count)
{
	if (pgf_reader_failed(rdr))
		return;

	gsize read = fread(buf, 1, count, rdr->in);
	if (read != count) {
		pgf_reader_error(rdr, 0, "Input error");
	}
}


static guint 
pgf_read_uint(PgfReader* rdr)
{
	guint u = 0;
	gint shift = 0;
	guint8 b = 0;
	do {
		b = pgf_read_u8(rdr);
		if (pgf_reader_failed(rdr)) 
			return 0;
		u |= (b & ~0x80) << shift;
		shift += 7;
	} while (b & 0x80);
	return u;
}

static gint
pgf_read_int(PgfReader* rdr)
{
	gint i = (gint)pgf_read_uint(rdr);
	return i;
}

static gint
pgf_read_len(PgfReader* rdr)
{
	gint len = pgf_read_int(rdr);
	if (pgf_reader_failed(rdr)) 
		// It's crucial that we return 0 on failure, so the
		// caller can proceed without checking for error
		// immediately.
		return 0;
	if (len < 0) {
		pgf_reader_error(rdr, 0, "Read invalid length: %d", len);
		return 0;
	}
	return len;
}

static glong
pgf_read_integer(PgfReader* rdr)
{
	guint8 tag = pgf_read_u8(rdr);
	switch (tag) {
	case 0:
		return pgf_read_i32be(rdr);
	case 1: ; // An actually legitimate use of the null statement!
		guint8 sign = pgf_read_u8(rdr);
		gint len = pgf_read_len(rdr);
		glong l = 0;
		for (gint i = 0; i < len; i++) {
			guint8 byte = pgf_read_u8(rdr);
			l += byte << (i * 8);
		}
		return sign == 1 ? l : -l;
	default:
		pgf_reader_tag_error(rdr, "Integer", tag);
		return 0;
	}
}


static double
pgf_read_double(PgfReader* rdr)
{
	glong mantissa = pgf_read_integer(rdr);
	glong exponent = pgf_read_int(rdr);

	if (pgf_reader_failed(rdr)) {
		return nan("");
	}
	
	return mantissa * exp2(exponent);
}

static void
pgf_read_unichar_into_string(PgfReader* rdr, GString* gstr)
{
	guint8 c = pgf_read_u8(rdr);
	if (pgf_reader_failed(rdr)) 
		return;
	
	g_string_append_c(gstr, (gchar)c);

	gint len = 0;

	if (c < 0x80) {
		len = 1;
	} else if (c < 0xc0) {
		goto err;
	} else if (c < 0xe0) {
		len = 2;
	} else if (c < 0xf0) {
		len = 3;
	} else if (c < 0xf5) {
		len = 4;
	} else {
		goto err;
	}

	for (gint i = 1; i < len; i++) {
		c = pgf_read_u8(rdr);
		if (c < 0x80 || c >= 0xc0) {
			goto err;
		}
		g_string_append_c(gstr, (gchar) c);
	}
	
	return;
err:
	pgf_reader_error(rdr, 0, "Unexpected UTF-8 byte: %02ux", c);
}

static GString*
pgf_read_tmp_string(PgfReader* rdr)
{
	gint len = pgf_read_len(rdr);
	GString* gstr = g_string_sized_new(len);

	for (gint i = 0; i < len; i++) {
		pgf_read_unichar_into_string(rdr, gstr);
	}

	return gstr;
}




///
/// Typeinfos
///

typedef struct PgfTypeBase PgfTypeBase;

typedef struct PgfPlacer PgfPlacer;

struct PgfPlacer {
	gpointer (*place)(PgfPlacer* placer, gsize size, gsize align);
};

static gpointer 
pgf_placer_place(PgfPlacer* placer, gsize size, gsize align)
{
	return placer->place(placer, size, align);
}

#define pgf_placer_place_type(placer, type)			\
	pgf_placer_place(placer, sizeof(type), gu_alignof(type))

#define pgf_placer_place_flex(placer, type, member, n_elems)   \
	pgf_placer_place(placer, GU_FLEX_SIZE(type, member, n_elems),	\
			 gu_alignof(type))

typedef struct PgfConstPlacer PgfConstPlacer;

struct PgfConstPlacer {
	PgfPlacer placer;
	gpointer p;
	gsize size;
};

static gpointer pgf_place_const(PgfPlacer* placer, 
				gsize size, 
				G_GNUC_UNUSED gsize align) 
{
	PgfConstPlacer* cplacer = GU_CONTAINER_P(placer, PgfConstPlacer, placer);
	g_assert(size == cplacer->size);
	return cplacer->p;
}


#define PGF_SIZED_CONST_PLACER(ptr, psize)		 \
	(&((PgfConstPlacer[1]) {			 \
	{					 	 \
		.placer = { .place = pgf_place_const },  \
		.p = (ptr),  		                 \
		.size = psize			 	 \
	}                                                \
				 })->placer)

#define PGF_CONST_PLACER(ptr) \
	PGF_SIZED_CONST_PLACER(ptr, sizeof(*(ptr)))


typedef struct PgfTypeFuncs PgfTypeFuncs;

struct PgfTypeFuncs {
	void (*unpickle)(const PgfTypeBase* base, PgfReader* rdr, 
			 PgfPlacer* placer);
	void (*dump)(const PgfTypeBase* base, gconstpointer v,
		     PgfWriter* wtr);
};

struct PgfTypeBase {
	gsize size; // not valid for flex structs
	gsize alignment;
	const PgfTypeFuncs* funcs;
};

#define PGF_TYPE_BASE(type, fns)	\
	{					\
		.size = sizeof(type),	\
		.alignment = gu_alignof(type),	\
		.funcs = fns	        \
	}

typedef struct PgfMiscType PgfMiscType;

struct PgfMiscType {
	PgfTypeBase b;
	const gchar* name;
};

#define PGF_MISC_TYPE(type, t_unpickle, t_dump) {		\
	.b = PGF_TYPE_BASE(type,				\
			   &GU_LVALUE(PgfTypeFuncs, {		\
		   .unpickle = t_unpickle GU_COMMA		\
		   .dump = t_dump				\
	})),							\
	.name = #type						\
}


static void 
pgf_unpickle(PgfReader* rdr, const PgfTypeBase* type, PgfPlacer* placer) {
	type->funcs->unpickle(type, rdr, placer);
}

static void 
pgf_dump(const PgfTypeBase* type, gconstpointer v, PgfWriter* out)
{
	type->funcs->dump(type, v, out);
}

#define PgfList(t)				\
	struct {				\
	gint len;				\
	const t* elems;				\
	}

#define PGF_LIST(t, ...)					\
	{							\
		.elems = (const t[]){__VA_ARGS__},			\
		.len = sizeof((const t[]){__VA_ARGS__}) / sizeof(t)	\
	}

#define PGF_PTRLIT(t, ...)			\
	((t[]){ __VA_ARGS__ })


// Struct types

typedef struct PgfMember PgfMember;

struct PgfMember {
	const gchar* name;
	goffset offset;
	const PgfTypeBase* type;
};

#define PGF_MEMBER_BASE(s_type, m_name, m_base) {		\
		.name = #m_name,				\
		.offset = offsetof(s_type, m_name),	\
			.type = m_base \
		}

#define PGF_MEMBER(s_type, m_name, m_type)	\
	PGF_MEMBER_BASE(s_type, m_name, m_type)

typedef PgfList(PgfMember) PgfMembers;

typedef struct PgfStructType PgfStructType;

struct PgfStructType {
	PgfTypeBase b;
	const gchar* name;
	PgfMembers members;
};

static const PgfMiscType pgf_length_type; // Initialized later

#define PGF_STRUCT_TYPE_WITH_FUNCS(s_type, funcs, ...) {	\
	.b = PGF_TYPE_BASE(s_type, funcs),			\
	.name = #s_type, \
	.members = PGF_LIST(PgfMember, __VA_ARGS__)		\
}

#define PGF_STRUCT_TYPE(s_type, ...) \
	PGF_STRUCT_TYPE_WITH_FUNCS(s_type, &pgf_struct_type_funcs, __VA_ARGS__)

#define PGF_STRUCT_TYPE_L(s_type, ...)		\
	GU_LVALUE(PgfStructType, PGF_STRUCT_TYPE(s_type, __VA_ARGS__))

static gpointer
pgf_unpickle_flex_member(const PgfTypeBase* type,
			 const PgfTypeBase* elem_type,
			 gint length, 
			 PgfReader* rdr,
			 PgfPlacer* placer)
{
	typedef guint8 Member[elem_type->size];
	guint8* p = pgf_placer_place(placer, 
				     type->size 
				     + length * sizeof(Member),
				     type->alignment);
	Member* members = (Member*)&p[type->size];
	for (gint j = 0; j < length; j++) {
		pgf_unpickle(rdr, elem_type, 
			     PGF_CONST_PLACER(&members[j]));
		if (pgf_reader_failed(rdr)) 
			return NULL;
	}
	return p;
}
			    

static void
pgf_unpickle_struct(const PgfTypeBase* info, PgfReader* rdr, PgfPlacer* placer)
{
	PgfStructType* sinfo = GU_CONTAINER_P(info, PgfStructType, b);
	guint8 buf[info->size];
	gint length = -1;
	guint8* p = NULL;
	pgf_debug("-> struct %s", sinfo->name);
	for (gint i = 0; i < sinfo->members.len; i++) {
		const PgfMember* m = &sinfo->members.elems[i];
		pgf_debug("-> %s.%s", sinfo->name, m->name);
		if ((gsize)m->offset == info->size) {
			g_assert(length >= 0 && p == NULL);
			p = pgf_unpickle_flex_member(info, m->type, length,
						     rdr, placer);
		} else {
			pgf_unpickle(rdr, m->type, 
				     PGF_SIZED_CONST_PLACER(&buf[m->offset],
							    m->type->size));
		}
		if (m->type == &pgf_length_type.b) {
			g_assert(length == -1);
			length = G_STRUCT_MEMBER(gint, buf, m->offset);
		}
		if (pgf_reader_failed(rdr)) 
			return;
		pgf_debug("<- %s.%s", sinfo->name, m->name);
	}
	if (p == NULL) {
		p = pgf_placer_place(placer, info->size, info->alignment);
	}
	memcpy(p, buf, info->size);
	pgf_debug("<- struct %s", sinfo->name);
}

static void
pgf_dump_elems(const PgfTypeBase* elem_type,
	       gint length, 
	       gconstpointer arr,
	       const gchar* anchor_prefix,
	       gint anchor_num,
	       PgfWriter* wtr)
{
	const guint8* p = arr;
	pgf_write_str(wtr, "[");
	for (gint i = 0; i < length; i++) {
		if (i > 0) {
			pgf_write_str(wtr, ", ");

		}
		if (anchor_prefix != NULL) {
			pgf_write_fmt(wtr, "&%s%d_%d ", 
				      anchor_prefix, anchor_num, i);
		}
		pgf_dump(elem_type, &p[i * elem_type->size], wtr);
	}
	pgf_write_str(wtr, "]");
}

static void
pgf_dump_struct(const PgfTypeBase* type, gconstpointer v, PgfWriter* wtr)
{
	PgfStructType* stype = GU_CONTAINER_P(type, PgfStructType, b);
	gint length = -1;
	const guint8* p = v;
	pgf_write_str(wtr, "{");
	gboolean first = TRUE;
	for (gint i = 0; i < stype->members.len; i++) {
		const PgfMember* m = &stype->members.elems[i];
		if (m->type == &pgf_length_type.b) {
			g_assert(length == -1);
			length = G_STRUCT_MEMBER(gint, p, m->offset);
			continue;
		}
		if (!first) {
			pgf_write_str(wtr, ", ");
		} else {
			first = FALSE;
		}
		pgf_write_fmt(wtr, "%s: ", m->name);
		if ((gsize)m->offset == type->size) {
			g_assert(length >= 0);
			pgf_dump_elems(m->type, length, &p[m->offset], 
				       NULL, 0, wtr);
		} else {
			pgf_dump(m->type, &p[m->offset], wtr);
		}
	}
	pgf_write_str(wtr, "}");
}


PgfTypeFuncs pgf_struct_type_funcs = { 
	.unpickle = pgf_unpickle_struct,
	.dump = pgf_dump_struct
};

// Parametric types

typedef struct PgfParametricType PgfParametricType;

struct PgfParametricType {
	PgfTypeBase b;
	const PgfTypeBase* type_arg;
};

#define PGF_PARAMETRIC_TYPE(type, funcs, param) { \
	.b = PGF_TYPE_BASE(type, funcs), \
	.type_arg = param \
 }

// Pointer types

typedef PgfParametricType PgfPointerType;

typedef struct PgfOwnedPlacer PgfOwnedPlacer;

struct PgfOwnedPlacer {
	PgfPlacer placer;
	gpointer* p;
	GuAllocator* ator;
};

static gpointer pgf_place_owned(PgfPlacer* placer, gsize size, gsize align) 
{
	PgfOwnedPlacer* pplacer = 
		GU_CONTAINER_P(placer, PgfOwnedPlacer, placer);
	*(pplacer->p) = gu_malloc_aligned(pplacer->ator, size, align);
	return *(pplacer->p);
}

static PgfOwnedPlacer pgf_owned_placer(GuAllocator* ator, gpointer* p)
{
	return (PgfOwnedPlacer) { { pgf_place_owned }, p, ator };
}

static void
pgf_unpickle_owned(const PgfTypeBase* info, PgfReader* rdr, PgfPlacer* placer)
{
	PgfPointerType* pinfo = GU_CONTAINER_P(info, PgfPointerType, b);
	gpointer* p = pgf_placer_place_type(placer, gpointer);
	PgfOwnedPlacer oplacer = pgf_owned_placer(rdr->ator, p);
	pgf_unpickle(rdr, pinfo->type_arg, &oplacer.placer);
}

static void
pgf_dump_pointer(G_GNUC_UNUSED const PgfTypeBase* type,
		 gconstpointer v, PgfWriter* wtr)
{
	PgfPointerType* ptype = GU_CONTAINER_P(type, PgfPointerType, b);
	const gpointer* p = v;
	pgf_dump(ptype->type_arg, *p, wtr);
}


static const PgfTypeFuncs pgf_owned_funcs = {
	.unpickle = pgf_unpickle_owned,
	.dump = pgf_dump_pointer
};

#define PGF_OWNED_TYPE(typedesc) \
	PGF_PARAMETRIC_TYPE(gpointer, &pgf_owned_funcs, typedesc)

#define PGF_OWNED_TYPE_L(typedesc)		\
	&GU_LVALUE(PgfPointerType, PGF_OWNED_TYPE(typedesc)).b



// Variants

typedef struct PgfVariantInfo PgfVariantInfo;
typedef struct PgfVariantType PgfVariantType;
typedef struct PgfVariantPlacer PgfVariantPlacer;
typedef struct PgfConstructor PgfConstructor;

struct PgfConstructor {
	gint c_tag;
	const gchar* name;
	const PgfTypeBase* type;
};

#define PGF_CONSTRUCTOR(ctag, c_name, c_type) {		\
		.c_tag = ctag,	 \
		.name = #c_name, \
		.type = c_type \
}

#define PGF_STRUCT_CONSTRUCTOR(c_tag, c_name, s_type, ...)	\
	PGF_CONSTRUCTOR(c_tag, c_name, 				\
			&PGF_STRUCT_TYPE_L(s_type, __VA_ARGS__).b)


	

typedef PgfList(PgfConstructor) PgfConstructors;

struct PgfVariantType {
	PgfTypeBase b;
	PgfConstructors ctors;
};

struct PgfVariantPlacer {
	PgfPlacer placer;
	guint c_tag;
	GuVariant* to;
	GuAllocator* ator;
};

static gpointer
pgf_place_variant(PgfPlacer* placer, gsize size, gsize align)
{
	PgfVariantPlacer* vplacer = 
		GU_CONTAINER_P(placer, PgfVariantPlacer, placer);
	return gu_variant_alloc(vplacer->ator, vplacer->c_tag, 
				size, align, vplacer->to);
}

static void
pgf_unpickle_variant(const PgfTypeBase* base, PgfReader* rdr, PgfPlacer* placer)
{
	const PgfVariantType* vtype = GU_CONTAINER_P(base, PgfVariantType, b);
	GuVariant* to = pgf_placer_place_type(placer, GuVariant);
	guint8 btag = pgf_read_u8(rdr);
	if (btag >= vtype->ctors.len) {
		pgf_reader_tag_error(rdr, "<variant>", btag);
		return;
	}
	const PgfConstructor* ctor = &vtype->ctors.elems[btag];
	PgfVariantPlacer vplacer = { { pgf_place_variant },
				     ctor->c_tag, to, rdr->ator };
	pgf_unpickle(rdr, ctor->type, &vplacer.placer);
}

static void
pgf_dump_variant(const PgfTypeBase* type, gconstpointer v, PgfWriter* wtr)
{
	const PgfVariantType* vtype = GU_CONTAINER_P(type, PgfVariantType, b);
	const GuVariant* p = v;
	gint c_tag = gu_variant_tag(*p);
	const PgfConstructor* ctor = NULL;
	for (gint i = 0; i < vtype->ctors.len; i++) {
		if (vtype->ctors.elems[i].c_tag == c_tag) {
			ctor = &vtype->ctors.elems[i];
			break;
		}
	}
	if (ctor == NULL) {
		pgf_writer_error(wtr, 0, "Unknown variant tag: %u", c_tag);
		return;
	}

	if (ctor->c_tag == c_tag) {
		pgf_write_fmt(wtr, "{%s: ", ctor->name);
		pgf_dump(ctor->type, gu_variant_data(*p), wtr);
		pgf_write_str(wtr, "}");
	}
}

static PgfTypeFuncs pgf_variant_funcs = {
	.unpickle = pgf_unpickle_variant,
	.dump = pgf_dump_variant
};

#define PGF_VARIANT_TYPE(type, ...) { \
		.b = PGF_TYPE_BASE(type, &pgf_variant_funcs), \
		.ctors = PGF_LIST(PgfConstructor, __VA_ARGS__) \
}

typedef PgfParametricType PgfVariantAsPtrType;

static void
pgf_unpickle_variant_as_ptr(const PgfTypeBase* type, 
			    PgfReader* rdr, PgfPlacer* placer)
{
	const PgfVariantAsPtrType* ptype = 
		GU_CONTAINER_P(type, const PgfVariantAsPtrType, b);
	g_assert(ptype->type_arg->funcs == &pgf_variant_funcs);
	GuVariant variant;
	pgf_unpickle(rdr, ptype->type_arg, PGF_CONST_PLACER(&variant));
	gpointer* to = pgf_placer_place_type(placer, gpointer);
	*to = (gpointer)variant.p;
}

static void
pgf_dump_variant_as_ptr(const PgfTypeBase* type, 
			gconstpointer v, PgfWriter* wtr)
{
	const PgfVariantAsPtrType* ptype = 
		GU_CONTAINER_P(type, const PgfVariantAsPtrType, b);
	GuVariant variant = { .p = (guintptr)*(gpointer*)v };
	pgf_dump(ptype->type_arg, &variant, wtr);
}

	
	
static const PgfTypeFuncs pgf_variant_as_ptr_funcs = {
	.unpickle = pgf_unpickle_variant_as_ptr,
	.dump = pgf_dump_variant_as_ptr
};

#define PGF_VARIANT_AS_PTR_TYPE(type) \
	PGF_PARAMETRIC_TYPE(gpointer, \
			    &pgf_variant_as_ptr_funcs, type)

#define PGF_VARIANT_AS_PTR_TYPE_L(type) \
	&GU_LVALUE(PgfVariantAsPtrType, PGF_VARIANT_AS_PTR_TYPE(type)).b



// Enums

typedef struct PgfEnumType PgfEnumType;
typedef struct PgfEnumerator PgfEnumerator;

typedef PgfList(PgfEnumerator) PgfEnumerators;

struct PgfEnumType {
	PgfTypeBase b;
	PgfEnumerators enumerators;
};

struct PgfEnumerator {
	gint val;
	const gchar* name;
};

#define PGF_ENUMERATOR(e_val, e_name) {		\
	.val = e_val,				\
	.name = #e_name 			\
}

static void
pgf_unpickle_enum(const PgfTypeBase* type, PgfReader* rdr, PgfPlacer* placer)
{
	// For now, assume that enum values are encoded in a single octet
	const PgfEnumType* etype = GU_CONTAINER_P(type, PgfEnumType, b);
	guint8 tag = pgf_read_u8(rdr);
	if (tag >= etype->enumerators.len) {
		pgf_reader_tag_error(rdr, "<enum>", tag);
	}
	if (pgf_reader_failed(rdr)) 
		return;
	const PgfEnumerator* enumerator = &etype->enumerators.elems[tag];
	gpointer to = pgf_placer_place(placer, type->size, type->alignment);
	if (type->size == sizeof(guint8)) {
		*((gint8*)to) = enumerator->val;
	} else if (type->size == sizeof(guint16)) {
		*((gint16*)to) = enumerator->val;
	} else if (type->size == sizeof(guint32)) {
		*((gint32*)to) = enumerator->val;
	} else if (type->size == sizeof(guint64)) {
		*((gint64*)to) = enumerator->val;
	} else {
		pgf_reader_error(rdr, 0, "Unknown size for enum: %ud", type->size);
	}
}

static void
pgf_dump_enum(const PgfTypeBase* type, gconstpointer v, PgfWriter* wtr)
{
	const PgfEnumType* etype = GU_CONTAINER_P(type, PgfEnumType, b);
	gint val = -1;
	switch (type->size) {
	case 1:
		val = *((const gint8*)v);
		break;
	case 2:
		val = *((const gint16*)v);
		break;
	case 4:
		val = *((const gint32*)v);
		break;
	case 8:
		val = *((const gint64*)v);
		break;
	default:
		pgf_writer_error(wtr, 0, "Unknown size for enum: %u", type->size);
		return;
	}
	for (gint i = 0; i < etype->enumerators.len; i++) {
		const PgfEnumerator* enumerator = &etype->enumerators.elems[i];
		if (enumerator->val == val) {
			pgf_write_str(wtr, enumerator->name);
			return;
		}
	}
	pgf_writer_error(wtr, 0, "Unknown enumeration value: %d", val);
}



PgfTypeFuncs pgf_enum_funcs = {
	.unpickle = pgf_unpickle_enum,
	.dump = pgf_dump_enum
};

#define PGF_ENUM_TYPE(type, ...) { \
	.b = PGF_TYPE_BASE(type, &pgf_enum_funcs),	    \
	.enumerators = PGF_LIST(PgfEnumerator, __VA_ARGS__) \
}

// Primitives

static void
pgf_unpickle_void(G_GNUC_UNUSED const PgfTypeBase* info, 
		  G_GNUC_UNUSED PgfReader* rdr,
		  G_GNUC_UNUSED PgfPlacer* placer)
{
}

static void
pgf_dump_void(G_GNUC_UNUSED const PgfTypeBase* info, 
	      G_GNUC_UNUSED gconstpointer v,
	      PgfWriter* wtr)
{
	pgf_write_str(wtr, "null");
}

static const PgfTypeFuncs pgf_void_funcs = {
	.unpickle = pgf_unpickle_void,
	.dump = pgf_dump_void
};

static const PgfMiscType pgf_void_type = {
	.b = { .size = 0, .alignment = 0, .funcs = &pgf_void_funcs },
	.name = "void"
};

static void
pgf_unpickle_int(const PgfTypeBase* type, 
		 PgfReader* rdr, PgfPlacer* placer) 
{
	const PgfMiscType* mtype = GU_CONTAINER_P(type, const PgfMiscType, b);
	pgf_debug("-> %s", mtype->name);
	gint* to = pgf_placer_place_type(placer, gint);
	*to = pgf_read_int(rdr);
	pgf_debug("<- %s: %d", mtype->name, *to);
}

static void
pgf_dump_int(G_GNUC_UNUSED const PgfTypeBase* info,
	     gconstpointer val, PgfWriter* wtr)
{
	const gint* ip = val;
	pgf_write_fmt(wtr, "%d", *ip);
}
	     

typedef PgfMiscType PgfIntType;

#define PGF_INT_TYPE(t)				\
	PGF_MISC_TYPE(t, pgf_unpickle_int, pgf_dump_int)

static const PgfIntType pgf_int_type =
	PGF_INT_TYPE(gint);


static void
pgf_unpickle_uint16be(G_GNUC_UNUSED const PgfTypeBase* info, 
		      PgfReader* rdr, PgfPlacer* placer)
{
	guint16* to = pgf_placer_place_type(placer, guint16);
	*to = pgf_read_u16be(rdr);
}

static void
pgf_dump_uint16(G_GNUC_UNUSED const PgfTypeBase* info,
		gconstpointer v, PgfWriter* wtr)
{
	const guint16* p = v;
	pgf_write_fmt(wtr, "%u", *p);
}


static const PgfMiscType pgf_uint16be_type = 
	PGF_MISC_TYPE(guint16, pgf_unpickle_uint16be, pgf_dump_uint16);

static void
pgf_unpickle_length(G_GNUC_UNUSED const PgfTypeBase* info, 
		    PgfReader* rdr, PgfPlacer* placer) 
{
	gint* to = pgf_placer_place_type(placer, gint);
	*to = pgf_read_len(rdr);
}

static const PgfMiscType pgf_length_type = 
	PGF_MISC_TYPE(PgfLength, pgf_unpickle_length, pgf_dump_int);

// 

static void
pgf_unpickle_double(G_GNUC_UNUSED const PgfTypeBase* info,
		    PgfReader* rdr, PgfPlacer* placer)
{
	gdouble* to = pgf_placer_place_type(placer, gdouble);
	*to = pgf_read_double(rdr);
}

static void
pgf_dump_double(G_GNUC_UNUSED const PgfTypeBase* info,
		gconstpointer v, PgfWriter* wtr)
{
	const gdouble* p = v;
	pgf_write_fmt(wtr, "%f", *p);
}

static const PgfMiscType pgf_double_type =
	PGF_MISC_TYPE(gdouble, pgf_unpickle_double, pgf_dump_double);

// int as a pointer

static void
pgf_unpickle_intptr(G_GNUC_UNUSED const PgfTypeBase* type,
		    PgfReader* rdr, PgfPlacer* placer)
{
	gint i = pgf_read_int(rdr);
	gpointer* to = pgf_placer_place_type(placer, gpointer);
	*to = GINT_TO_POINTER(i);
}

static void
pgf_dump_intptr(G_GNUC_UNUSED const PgfTypeBase* type,
		gconstpointer v, PgfWriter* wtr)
{
	const gpointer* p = v;
	pgf_write_fmt(wtr, "%d", GPOINTER_TO_INT(*p));
}
 
static const PgfMiscType pgf_intptr_type = 
	PGF_MISC_TYPE(gpointer, pgf_unpickle_intptr, pgf_dump_intptr);


// Maybe
typedef PgfParametricType PgfMaybeType;

// works only for otherwise non-null pointers
static void
pgf_unpickle_maybe(const PgfTypeBase* base, PgfReader* rdr, PgfPlacer* placer)
{
	PgfMaybeType* mtype =
		GU_CONTAINER_P(base, PgfMaybeType, b);
	guint8 tag = pgf_read_u8(rdr);
	switch (tag) {
	case 0: ;
		gpointer* to = pgf_placer_place_type(placer, gpointer);
		*to = NULL;
		break;
	case 1:
		pgf_unpickle(rdr, mtype->type_arg, placer);
		break;
	default:
		pgf_reader_tag_error(rdr, "<maybe>", tag);
		break;
	}
}

static void
pgf_dump_maybe(const PgfTypeBase* base, gconstpointer v, PgfWriter* wtr)
{
	PgfMaybeType* mtype = GU_CONTAINER_P(base, PgfMaybeType, b);
	const gpointer* p = v;
	if (*p == NULL) {
		pgf_write_str(wtr, "null");
	} else {
		pgf_dump(mtype->type_arg, v, wtr);
	}
}


static const PgfTypeFuncs pgf_maybe_type_funcs = {
	.unpickle = pgf_unpickle_maybe,
	.dump = pgf_dump_maybe
};

#define PGF_MAYBE_P_TYPE(p_elem_type) \
	PGF_PARAMETRIC_TYPE(gpointer, &pgf_maybe_type_funcs, \
			    p_elem_type)

#define PGF_MAYBE_P_TYPE_L(p_elem_type)		\
	&GU_LVALUE(PgfMaybeType, PGF_MAYBE_P_TYPE(p_elem_type)).b


// Lists

typedef PgfStructType PgfListType;

typedef struct {
	gint len;
	const guint8* elems;
	const PgfTypeBase* elem_type;
} PgfListInfo;

static PgfListInfo
pgf_list_open(const PgfListType* ltype, gconstpointer l)
{
	const guint8* p = l;
	g_assert(ltype->members.len == 2);
	const PgfMember* len_mem = &ltype->members.elems[0];
	const PgfMember* elems_mem = &ltype->members.elems[1];
	g_assert(len_mem->type == &pgf_length_type.b);
	g_assert(elems_mem->offset == (goffset)ltype->b.size);
	PgfListInfo ret = {
		.len = *(gint*)&p[len_mem->offset],
		.elems = &p[elems_mem->offset],
		.elem_type = elems_mem->type
	};
	return ret;
}


static void
pgf_dump_list(const PgfTypeBase* type, gconstpointer v, PgfWriter* wtr)
{
	const PgfListType* ltype = GU_CONTAINER_P(type, PgfListType, b);
	PgfListInfo linfo = pgf_list_open(ltype, v);
	pgf_dump_elems(linfo.elem_type, linfo.len, linfo.elems, NULL, 0, wtr);
}

static const PgfTypeFuncs pgf_list_type_funcs = {
	.unpickle = pgf_unpickle_struct,
	.dump = pgf_dump_list
};


#define PGF_LIST_TYPE_WITH_FUNCS(ctype, p_elem_type, funcs)	   \
	PGF_STRUCT_TYPE_WITH_FUNCS(ctype, funcs,		   \
		PGF_MEMBER(ctype, len, &pgf_length_type.b),	   \
	   	PGF_MEMBER(ctype, elems, p_elem_type))

#define PGF_LIST_TYPE(ctype, p_elem_type)	\
	PGF_LIST_TYPE_WITH_FUNCS(ctype, p_elem_type, &pgf_list_type_funcs)

#define PGF_LIST_TYPE_L(ctype, p_elem_type)				\
	&GU_LVALUE(PgfListType, PGF_LIST_TYPE(ctype, p_elem_type)).b

#define PGF_OWNED_LIST_TYPE(ctype, elemtype)		\
	PGF_OWNED_TYPE(PGF_LIST_TYPE_L(ctype, elemtype))

#define PGF_OWNED_LIST_TYPE_L(ctype, elemtype)		\
	PGF_OWNED_TYPE_L(PGF_LIST_TYPE_L(ctype, elemtype))

// Maps

typedef struct PgfMapType PgfMapType;

struct PgfMapType {
	PgfTypeBase b;
	const PgfTypeBase* key_type;
	const PgfTypeBase* value_type;
	GHashFunc hash_func;
	GEqualFunc compare_func;
};

static void
pgf_unpickle_map_p(const PgfTypeBase* info, PgfReader* rdr, PgfPlacer* placer)
{
	PgfMapType* minfo = GU_CONTAINER_P(info, PgfMapType, b);
	GHashTable* ht = g_hash_table_new(minfo->hash_func,
					  minfo->compare_func);
	gint len = 0;
	gu_mem_pool_register_finalizer(rdr->pool,
				       (GDestroyNotify)g_hash_table_destroy,
				       ht);
	pgf_unpickle(rdr, &pgf_length_type.b, PGF_CONST_PLACER(&len));

	for (gint i = 0; i < len; i++) {
		gpointer key = NULL;
		pgf_unpickle(rdr, minfo->key_type, 
			     PGF_CONST_PLACER(&key));
		if (pgf_reader_failed(rdr)) 
			return;
		gpointer value = NULL;
		pgf_unpickle(rdr, minfo->value_type, 
			     PGF_CONST_PLACER(&value));
		if (pgf_reader_failed(rdr)) 
			return;
		g_hash_table_insert(ht, key, value);
	}
	
	GHashTable** htp = pgf_placer_place_type(placer, GHashTable*);
	*htp = ht;
}

typedef struct {
	const PgfMapType* mtype;
	PgfWriter* wtr;
	gboolean first;
} PgfHashDumpContext;

static void
pgf_dump_hash_entry(gpointer key, gpointer value, gpointer user_data)
{
	PgfHashDumpContext* ctx = user_data;
	if (ctx->first) {
		ctx->first = FALSE;
	} else {
		pgf_write_str(ctx->wtr, ", ");
	}
	pgf_dump(ctx->mtype->key_type, &key, ctx->wtr);
	pgf_write_str(ctx->wtr, ": ");
	pgf_dump(ctx->mtype->value_type, &value, ctx->wtr);
}

static void
pgf_dump_map_p(const PgfTypeBase* info,
	       gconstpointer v, PgfWriter* wtr)
{
	PgfMapType* mtype = GU_CONTAINER_P(info, PgfMapType, b);
	// glib doesn't support const hashtables, so must cast const away
	GHashTable* ht = *(GHashTable**)v; 
	pgf_write_str(wtr, "{");
	PgfHashDumpContext ctx = { mtype, wtr, TRUE };
	g_hash_table_foreach(ht, pgf_dump_hash_entry, &ctx);
	pgf_write_str(wtr, "}");
}


static const PgfTypeFuncs pgf_map_p_type_funcs = {
	.unpickle = pgf_unpickle_map_p,
	.dump = pgf_dump_map_p
};

#define PGF_MAP_P_TYPE(key_t, val_t, hash_fn, cmp_fn) {			\
	.b = PGF_TYPE_BASE(GHashTable*, &pgf_map_p_type_funcs), \
	.key_type = key_t, \
	.value_type = val_t, \
	.hash_func = hash_fn, \
	.compare_func = cmp_fn \
}

// GuString

static void
pgf_unpickle_string_p(G_GNUC_UNUSED const PgfTypeBase* type, 
		      PgfReader* rdr, PgfPlacer* placer)
{
	pgf_debug("-> GuString*");
	gint len = pgf_read_len(rdr);
	GuString* tmp = gu_list_new(GuString, NULL, len);
	tmp->len = len;
	pgf_read_chars(rdr, tmp->elems, len);
	GuString* interned = g_hash_table_lookup(rdr->interned_strings, tmp);
	if (interned == NULL) {
		interned = gu_list_new(GuString, rdr->ator, len);
		memcpy(interned->elems, tmp->elems, tmp->len);
		g_hash_table_insert(rdr->interned_strings, 
				    interned, interned);
	}
	g_free(tmp);
	GuString** to = pgf_placer_place_type(placer, GuString*);
	*to = interned;
	pgf_debug("<- GuString*: %.*s", interned->len, interned->elems);
}

static void
pgf_dump_string_p(G_GNUC_UNUSED const PgfTypeBase* info,
		  gconstpointer v, PgfWriter* wtr)
{
	GuString* const* p = v;
	pgf_write_string(wtr, *p);
}

static PgfMiscType pgf_string_p_type =
	PGF_MISC_TYPE(GuString*, pgf_unpickle_string_p, pgf_dump_string_p);


// Ids


typedef struct PgfCachedPlacer PgfCachedPlacer;

struct PgfCachedPlacer {
	PgfPlacer placer;
	PgfPlacer* real_placer;
	gpointer p;
};

static gpointer
pgf_place_cached(PgfPlacer* placer, gsize size, gsize align)
{
	PgfCachedPlacer* cplacer = 
		GU_CONTAINER_P(placer, PgfCachedPlacer, placer);
	cplacer->p = pgf_placer_place(cplacer->real_placer, size, align);
	return cplacer->p;
}

typedef struct PgfIdArrayType PgfIdArrayType;

struct PgfIdArrayType {
	PgfListType l;
	goffset ctx_offset;
	const gchar* anchor_prefix;
};

static void
pgf_unpickle_idarray(const PgfTypeBase* type,
		     PgfReader* rdr, PgfPlacer* placer)
{
	const PgfListType* ltype = 
		GU_CONTAINER_P(type, const PgfListType, b);
	const PgfIdArrayType* itype = 
		GU_CONTAINER_P(ltype, const PgfIdArrayType, l);
	PgfCachedPlacer cplacer = { { pgf_place_cached }, placer, NULL };
	pgf_unpickle_struct(type, rdr, &cplacer.placer);
	PgfIdContext* ctx = G_STRUCT_MEMBER_P(&rdr->ctx, itype->ctx_offset);
	ctx->count++;
	ctx->current = cplacer.p;
}

static void
pgf_dump_idarray(const PgfTypeBase* type, gconstpointer v, PgfWriter* wtr)
{
	const PgfListType* ltype = 
		GU_CONTAINER_P(type, const PgfListType, b);
	const PgfIdArrayType* itype = 
		GU_CONTAINER_P(ltype, const PgfIdArrayType, l);
	PgfListInfo linfo = pgf_list_open(ltype, v);
	PgfIdContext* ctx = G_STRUCT_MEMBER_P(&wtr->ctx, itype->ctx_offset);
	ctx->count++;
	ctx->current = v;
	pgf_dump_elems(linfo.elem_type, linfo.len, linfo.elems, 
		       itype->anchor_prefix, ctx->count, wtr);
}

static const PgfTypeFuncs pgf_id_array_funcs = {
	.unpickle = pgf_unpickle_idarray,
	.dump = pgf_dump_idarray
};

#define PGF_ID_ARRAY_TYPE(ctype, e_type, ctx_member, anch_pfx) {	\
	.l = PGF_LIST_TYPE_WITH_FUNCS(ctype, e_type, &pgf_id_array_funcs), \
	.ctx_offset = offsetof(PgfContext, ctx_member),	\
	.anchor_prefix = anch_pfx \
}

typedef struct PgfIdType PgfIdType;

struct PgfIdType {
	PgfTypeBase b;
	const PgfIdArrayType* id_array_type;
};

static void
pgf_unpickle_id(const PgfTypeBase* type, PgfReader* rdr, PgfPlacer* placer)
{
	const PgfIdArrayType* itype = 
		GU_CONTAINER_P(type, PgfIdType, b)->id_array_type;
	gint id = pgf_read_len(rdr);
	PgfIdContext* ctx =
		G_STRUCT_MEMBER_P(&rdr->ctx, itype->ctx_offset);
	PgfListInfo linfo = pgf_list_open(&itype->l, ctx->current);
	if (id >= linfo.len) {
		pgf_reader_error(rdr, 0, "Invalid id: %d", id);
	}
	if (pgf_reader_failed(rdr))
		return;
	gconstpointer* to = pgf_placer_place_type(placer, gconstpointer);
	*to = &linfo.elems[id * linfo.elem_type->size];
}

static void
pgf_dump_id(const PgfTypeBase* type, gconstpointer v, PgfWriter* wtr)
{
	const PgfIdArrayType* itype = 
		GU_CONTAINER_P(type, const PgfIdType, b)->id_array_type;
	PgfIdContext* ctx =
		G_STRUCT_MEMBER_P(&wtr->ctx, itype->ctx_offset);
	PgfListInfo linfo = pgf_list_open(&itype->l, ctx->current);
	guint8* const * p = v;
	gint id = (*p - linfo.elems) / linfo.elem_type->size;
	g_assert(id >= 0 && id < linfo.len);
	pgf_write_fmt(wtr, "*%s%d_%d", 
		      itype->anchor_prefix, ctx->count, id);
}

static const PgfTypeFuncs pgf_id_funcs = {
	.unpickle = pgf_unpickle_id,
	.dump = pgf_dump_id
};

#define PGF_ID_TYPE(iatype) { \
	.b = PGF_TYPE_BASE(gpointer, &pgf_id_funcs), \
	.id_array_type = iatype \
}




//
// Type definitions 
//

static const PgfVariantType pgf_literal_type = PGF_VARIANT_TYPE(
	PgfLiteral,
	PGF_CONSTRUCTOR(PGF_LITERAL_STR, Str, &pgf_string_p_type.b),
	PGF_CONSTRUCTOR(PGF_LITERAL_INT, Int, &pgf_int_type.b),
	PGF_CONSTRUCTOR(PGF_LITERAL_FLT, Flt, &pgf_double_type.b));

static const PgfEnumType pgf_bind_type_type = PGF_ENUM_TYPE(
	PgfBindType,
	PGF_ENUMERATOR(PGF_BIND_TYPE_EXPLICIT, Explicit),
	PGF_ENUMERATOR(PGF_BIND_TYPE_IMPLICIT, Implicit));

static const PgfStructType pgf_type_type;
static const PgfPointerType pgf_type_p_type;

static const PgfStructType pgf_hypo_type = PGF_STRUCT_TYPE(
	PgfHypo,
	PGF_MEMBER(PgfHypo, bindtype, &pgf_bind_type_type.b),
	PGF_MEMBER(PgfHypo, cid, &pgf_string_p_type.b),
	PGF_MEMBER(PgfHypo, type, &pgf_type_p_type.b));

static const PgfPointerType pgf_hypos_p_type =
	PGF_OWNED_LIST_TYPE(PgfHypos, &pgf_hypo_type.b);

static const PgfVariantType pgf_expr_type;

static const PgfStructType pgf_type_type = PGF_STRUCT_TYPE(
	PgfType,
	PGF_MEMBER(PgfType, hypos, &pgf_hypos_p_type.b),
	PGF_MEMBER(PgfType, cid, &pgf_string_p_type.b),
	PGF_MEMBER(PgfType, n_exprs, &pgf_length_type.b),
	PGF_MEMBER(PgfType, exprs, &pgf_expr_type.b)
);

static const PgfIntType pgf_fid_type =
	PGF_INT_TYPE(PgfFId);

static const PgfIntType pgf_metaid_type =
	PGF_INT_TYPE(PgfMetaId);

static const PgfIntType pgf_id_type =
	PGF_INT_TYPE(PgfId);


static const PgfPointerType pgf_type_p_type = 
	PGF_OWNED_TYPE(&pgf_type_type.b);

static const PgfVariantType pgf_expr_type = PGF_VARIANT_TYPE(
	PgfExpr,
	PGF_STRUCT_CONSTRUCTOR(
		PGF_EXPR_ABS, Abs, PgfExprAbs,
		PGF_MEMBER(PgfExprAbs, bind_type, &pgf_bind_type_type.b),
		PGF_MEMBER(PgfExprAbs, id, &pgf_string_p_type.b),
		PGF_MEMBER(PgfExprAbs, body, &pgf_expr_type.b)),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_EXPR_APP, App, PgfExprApp,
		PGF_MEMBER(PgfExprApp, fun, &pgf_expr_type.b),
		PGF_MEMBER(PgfExprApp, arg, &pgf_expr_type.b)),
	PGF_CONSTRUCTOR(
		PGF_EXPR_LIT, Lit, &pgf_literal_type.b),
	PGF_CONSTRUCTOR(
		PGF_EXPR_META, Meta, &pgf_int_type.b),
	PGF_CONSTRUCTOR(
		PGF_EXPR_FUN, Fun, &pgf_string_p_type.b),
	PGF_CONSTRUCTOR(
		PGF_EXPR_VAR, Var, &pgf_int_type.b),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_EXPR_TYPED, Typed, PgfExprTyped,
		PGF_MEMBER(PgfExprTyped, expr, &pgf_expr_type.b),
		PGF_MEMBER(PgfExprTyped, type, &pgf_type_p_type.b)),
	PGF_CONSTRUCTOR(
		PGF_EXPR_IMPL_ARG, ImplArg, &pgf_expr_type.b));


#define PGF_CIDMAP_P_TYPE(val_type)			\
	PGF_MAP_P_TYPE(&pgf_string_p_type.b, val_type,	\
		       gu_string_hash, gu_string_equal)

#define PGF_CIDMAP_P_TYPE_L(val_type)				\
	&GU_LVALUE(PgfMapType, PGF_CIDMAP_P_TYPE(val_type)).b

#define PGF_INTMAP_P_TYPE(val_type)		\
	PGF_MAP_P_TYPE(&pgf_intptr_type.b, val_type, NULL, NULL)

#define PGF_INTMAP_P_TYPE_L(val_type)		\
	&GU_LVALUE(PgfMapType, PGF_INTMAP_P_TYPE(val_type)).b


static const PgfMapType pgf_flags_p_type =
	PGF_CIDMAP_P_TYPE(PGF_VARIANT_AS_PTR_TYPE_L(&pgf_literal_type.b));

static const PgfPointerType pgf_strings_p_type =
	PGF_OWNED_LIST_TYPE(GuStrings, &pgf_string_p_type.b);

static const PgfStructType pgf_alternative_type = PGF_STRUCT_TYPE(
	PgfAlternative,
	PGF_MEMBER(PgfAlternative, s1, &pgf_strings_p_type.b),
	PGF_MEMBER(PgfAlternative, s2, &pgf_strings_p_type.b));

static const PgfVariantType pgf_symbol_type = PGF_VARIANT_TYPE(
	PgfSymbol,
	PGF_STRUCT_CONSTRUCTOR(
		PGF_SYMBOL_CAT, SymCat, PgfSymbolCat,
		PGF_MEMBER(PgfSymbolCat, d, &pgf_int_type.b),
		PGF_MEMBER(PgfSymbolCat, r, &pgf_int_type.b)),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_SYMBOL_LIT, SymLit, PgfSymbolLit,
		PGF_MEMBER(PgfSymbolLit, d, &pgf_int_type.b),
		PGF_MEMBER(PgfSymbolLit, r, &pgf_int_type.b)),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_SYMBOL_VAR, SymVar, PgfSymbolVar,
		PGF_MEMBER(PgfSymbolVar, d, &pgf_int_type.b), 
		PGF_MEMBER(PgfSymbolVar, r, &pgf_int_type.b)),
	PGF_CONSTRUCTOR(
		PGF_SYMBOL_KS, SymKS, &pgf_strings_p_type.b),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_SYMBOL_KP, SymKP, PgfSymbolKP,
		PGF_MEMBER(PgfSymbolKP, ts, &pgf_strings_p_type.b),
		PGF_MEMBER(PgfSymbolKP, n_alts, &pgf_length_type.b),
		PGF_MEMBER(PgfSymbolKP, alts, &pgf_alternative_type.b)));


static const PgfStructType pgf_cnccat_type = PGF_STRUCT_TYPE(
	PgfCncCat,
	PGF_MEMBER(PgfCncCat, fid1, &pgf_int_type.b),
	PGF_MEMBER(PgfCncCat, fid2, &pgf_int_type.b),
	PGF_MEMBER(PgfCncCat, n_strings, &pgf_length_type.b),
	PGF_MEMBER(PgfCncCat, strings, &pgf_string_p_type.b));

static const PgfListType pgf_sequence_type =
	PGF_LIST_TYPE(PgfSequence, PGF_OWNED_TYPE_L(&pgf_symbol_type.b));

static const PgfIdArrayType pgf_sequences_type = 
	PGF_ID_ARRAY_TYPE(PgfSequences,
			  PGF_OWNED_TYPE_L(&pgf_sequence_type.b), 
			  sequences, "S");

static const PgfIdType pgf_sequence_id_type = 
	PGF_ID_TYPE(&pgf_sequences_type);

static const PgfStructType pgf_cncfun_type = PGF_STRUCT_TYPE(
	PgfCncFun,
	PGF_MEMBER(PgfCncFun, fun, &pgf_string_p_type.b),
	PGF_MEMBER(PgfCncFun, n_lins, &pgf_length_type.b),
	PGF_MEMBER(PgfCncFun, lins, &pgf_sequence_id_type.b));

static const PgfIdArrayType pgf_cncfuns_type = 
	PGF_ID_ARRAY_TYPE(PgfCncFuns,
			  PGF_OWNED_TYPE_L(&pgf_cncfun_type.b), 
			  cncfuns, "F");

static const PgfIdType pgf_funid_type = 
	PGF_ID_TYPE(&pgf_cncfuns_type);

static const PgfStructType pgf_parg_type = PGF_STRUCT_TYPE(
	PgfPArg,
	PGF_MEMBER(PgfPArg, n_hypos, &pgf_length_type.b),
	PGF_MEMBER(PgfPArg, hypos, &pgf_fid_type.b),
	PGF_MEMBER(PgfPArg, fid, &pgf_fid_type.b));

static const PgfVariantType pgf_production_type = PGF_VARIANT_TYPE(
	PgfProduction,
	PGF_STRUCT_CONSTRUCTOR(
		PGF_PRODUCTION_APPLY, PApply, PgfProductionApply,
		PGF_MEMBER(PgfProductionApply, fun, &pgf_funid_type.b),
		PGF_MEMBER(PgfProductionApply, n_args, &pgf_length_type.b),
		PGF_MEMBER(PgfProductionApply, args, 
			   PGF_OWNED_TYPE_L(&pgf_parg_type.b))),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_PRODUCTION_COERCE, PCoerce, PgfProductionCoerce,
		PGF_MEMBER(PgfProductionCoerce, coerce, &pgf_fid_type.b)),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_PRODUCTION_CONST, PConst, PgfProductionConst,
		PGF_MEMBER(PgfProductionConst, expr, &pgf_expr_type.b),
		PGF_MEMBER(PgfProductionConst, n_toks, &pgf_length_type.b),
		PGF_MEMBER(PgfProductionConst, toks, &pgf_string_p_type.b)));

static const PgfVariantType pgf_patt_type = PGF_VARIANT_TYPE(
	PgfPatt,
	PGF_STRUCT_CONSTRUCTOR(
		PGF_PATT_APP, PApp, PgfPattApp,
		PGF_MEMBER(PgfPattApp, ctor, &pgf_string_p_type.b),
		PGF_MEMBER(PgfPattApp, n_args, &pgf_length_type.b),
		PGF_MEMBER(PgfPattApp, args, &pgf_patt_type.b)),
	PGF_CONSTRUCTOR(
		PGF_PATT_LIT, PLit, 
		PGF_OWNED_TYPE_L(&pgf_literal_type.b)),
	PGF_CONSTRUCTOR(
		PGF_PATT_VAR, PVar, 
		&pgf_string_p_type.b),
	PGF_STRUCT_CONSTRUCTOR(
		PGF_PATT_AS, PAs, PgfPattAs,
		PGF_MEMBER(PgfPattAs, var, &pgf_string_p_type.b),
		PGF_MEMBER(PgfPattAs, patt, &pgf_patt_type.b)),
	PGF_CONSTRUCTOR(
		PGF_PATT_WILD, PWild, &pgf_void_type.b),
	PGF_CONSTRUCTOR(
		PGF_PATT_IMPL_ARG, PImplArg, &pgf_patt_type.b),
	PGF_CONSTRUCTOR(
		PGF_PATT_TILDE, PTilde, &pgf_expr_type.b));

static const PgfStructType pgf_equation_type = PGF_STRUCT_TYPE(
	PgfEquation,
	PGF_MEMBER(PgfEquation, body, &pgf_expr_type.b),
	PGF_MEMBER(PgfEquation, n_patts, &pgf_length_type.b),
	PGF_MEMBER(PgfEquation, patts, &pgf_patt_type.b));

static const PgfStructType pgf_fundecl_type = PGF_STRUCT_TYPE(
	PgfFunDecl,
	PGF_MEMBER(PgfFunDecl, type, &pgf_type_p_type.b),
	PGF_MEMBER(PgfFunDecl, arity, &pgf_int_type.b),
	PGF_MEMBER(PgfFunDecl, defns,
		   PGF_MAYBE_P_TYPE_L(
			   PGF_OWNED_LIST_TYPE_L(PgfEquations,
				   PGF_OWNED_TYPE_L(&pgf_equation_type.b)))));

static const PgfStructType pgf_cat_type = PGF_STRUCT_TYPE(
	PgfCat,
	PGF_MEMBER(PgfCat, context, &pgf_hypos_p_type.b),
	PGF_MEMBER(PgfCat, n_functions, &pgf_length_type.b),
	PGF_MEMBER(PgfCat, functions, &pgf_string_p_type.b));

static const PgfStructType pgf_abstr_type = PGF_STRUCT_TYPE(
	PgfAbstr,
	PGF_MEMBER(PgfAbstr, aflags, &pgf_flags_p_type.b),
	PGF_MEMBER(PgfAbstr, funs,
		   PGF_CIDMAP_P_TYPE_L(PGF_OWNED_TYPE_L(&pgf_fundecl_type.b))),
	PGF_MEMBER(PgfAbstr, cats,
		   PGF_CIDMAP_P_TYPE_L(PGF_OWNED_TYPE_L(&pgf_cat_type.b))));




static const PgfStructType pgf_concr_type = PGF_STRUCT_TYPE(
	PgfConcr,
	PGF_MEMBER(PgfConcr, cflags, &pgf_flags_p_type.b),
	PGF_MEMBER(PgfConcr, printnames,
		   PGF_CIDMAP_P_TYPE_L(&pgf_string_p_type.b)),
	PGF_MEMBER(PgfConcr, sequences, 
		   PGF_OWNED_TYPE_L(&pgf_sequences_type.l.b)),
	PGF_MEMBER(PgfConcr, cncfuns, 
		   PGF_OWNED_TYPE_L(&pgf_cncfuns_type.l.b)),
	PGF_MEMBER(PgfConcr, lindefs, 
		   PGF_INTMAP_P_TYPE_L(
			   PGF_OWNED_LIST_TYPE_L(PgfFunIds,
						 &pgf_funid_type.b))),
	PGF_MEMBER(PgfConcr, productions,
		   PGF_INTMAP_P_TYPE_L(
			   PGF_OWNED_LIST_TYPE_L(PgfProductions,
						 &pgf_production_type.b))),
	PGF_MEMBER(PgfConcr, cnccats,
		   PGF_CIDMAP_P_TYPE_L(PGF_OWNED_TYPE_L(&pgf_cnccat_type.b))),
	PGF_MEMBER(PgfConcr, totalcats, &pgf_fid_type.b));

static const PgfStructType pgf_pgf_type = PGF_STRUCT_TYPE(
	PgfPGF,
	PGF_MEMBER(PgfPGF, major_version, &pgf_uint16be_type.b),
	PGF_MEMBER(PgfPGF, minor_version, &pgf_uint16be_type.b),
	PGF_MEMBER(PgfPGF, gflags, &pgf_flags_p_type.b),
	PGF_MEMBER(PgfPGF, absname, &pgf_string_p_type.b),
	PGF_MEMBER(PgfPGF, abstract, &pgf_abstr_type.b),
	PGF_MEMBER(PgfPGF, concretes,
		   PGF_CIDMAP_P_TYPE_L(PGF_OWNED_TYPE_L(&pgf_concr_type.b))));


PgfPGF* pgf_read(FILE* in, GError** err_out)
{
	GuMemPool* pool = gu_mem_pool_new();
	PgfReader* rdr = pgf_reader_new(in, pool);
	PgfPGF* pgf = NULL;
	pgf_unpickle(rdr, 
		     PGF_OWNED_TYPE_L(&pgf_pgf_type.b),
		     PGF_CONST_PLACER(&pgf));
	if (rdr->err != NULL) {
		g_propagate_error(err_out, rdr->err);
		rdr->err = NULL;
		pgf = NULL;
	}
	pgf_reader_free(rdr);
	if (pgf == NULL) {
		gu_mem_pool_free(pool);
		return NULL;
	}
	pgf->pool = pool;
	return pgf;
}

void pgf_write_yaml(PgfPGF* pgf, FILE* to, GError** err_out)
{
	PgfWriter wtr = {
		.out = to,
		.err = NULL,
		.ctx = { { 0, NULL}, { 0, NULL } }
	};
	pgf_dump(&pgf_pgf_type.b, pgf, &wtr);
	if (wtr.err != NULL) {
		g_propagate_error(err_out, wtr.err);
	}
}

void pgf_free(PgfPGF* pgf) {
	gu_mem_pool_free(pgf->pool);
}

// These are needed to make the compiler include the debug information
// for these types

static const PgfConcr* pgf_reader_dummy_1;
static const PgfType* pgf_reader_dummy_2;
