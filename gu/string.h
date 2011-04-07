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

#ifndef GU_STRING_H_
#define GU_STRING_H_

#include <stdarg.h>
#include <gu/mem.h>

// This is not fully portable: GuString* pointers are actually char
// pointers so they are char-aligned, whereas a struct pointer might
// have a different representation. But typedefing a GuString to char 
// or void diminishes type safety.
//
// We could of course create a struct for the pointers themselves, but
// then we lose the convenient void*-compatibility,

typedef struct GuString GuString;

typedef const GuString GuCString;

typedef GuString* GuStringP;
typedef GuCString* GuCStringP;

unsigned 
gu_string_hash(const void* s);

gboolean 
gu_string_equal(const void* s1, const void* s2);

GuString* 
gu_string_new(GuPool* pool, int len);

GuString* 
gu_string_new_c(GuPool* pool, const char* cstr);

GuString* 
gu_string_copy(GuPool* pool, const GuString* from);

GuString*
gu_string_format(GuPool* pool, const char* fmt, ...) G_GNUC_PRINTF(2,3);

GuString*
gu_string_format_v(GuPool* pool, const char* fmt, va_list args);

inline int
gu_string_length(const GuString* s);

inline char*
gu_string_data(GuString* s);

inline const char*
gu_string_cdata(const GuString* s);





#define GU_STRING_FMT "%.*s"
#define GU_STRING_FMT_ARGS(s)			\
	gu_string_length(s), gu_string_cdata(s)




#include <gu/type.h>
extern GU_DECLARE_TYPE(GuString, abstract);
extern GU_DECLARE_TYPE(GuStringP, pointer);



// internal

#include <limits.h>


#define GuStringShortN_(len_)				\
	struct {					\
		unsigned char len;			\
		char data[len_];		\
	}
#define GuStringLongN_(len_)				\
	struct {					\
		int true_len;				\
		GuStringShortN_(len_) ss;		\
	}

typedef GuStringLongN_(1) GuStringEmpty_;

extern const GuStringEmpty_ gu_string_empty_;

#define gu_string_empty ((const GuString*)&gu_string_empty_.ss)

#define GU_STRING_SHORTN_INIT_(cstr_,len_) {			\
		.len = (unsigned char)len_,			\
		.data = cstr_,				\
}

#define gu_string_short_(qual_,cstr_,len_)			\
       (&(qual_ GuStringShortN_(len_)) 	\
       GU_STRING_SHORTN_INIT_(cstr_,len_)	\
	)

#define GU_STRING_LONGN_INIT_(cstr_,len_) {			\
		.true_len = len_,				\
		.ss = GU_STRING_SHORTN_INIT_(cstr_,0),	\
}

#define gu_string_long_(qual_,cstr_,len_)			\
       (&(qual_ GuStringLongN_(len_))	\
	       GU_STRING_LONGN_INIT_(cstr_,len_)	\
		)

#define GU_DEFINE_ATOM_(name_, cstr_, len_)		\
	const GuStringShortN_(len_)			\
	gu_atom_##name_##_ = GU_STRING_SHORTN_INIT_(cstr_, len_)

#define GU_DEFINE_ATOM(name_, cstr_) \
	GU_DEFINE_ATOM_(name_, cstr_, sizeof(cstr_)-1)

#define gu_atom(name_)				\
	((const GuString*)&gu_atom_##name_##_)

#define gu_string__(qual_,cstr_,len_)					\
	(len_ == 0							\
	 ? (qual_ GuString*) &gu_string_empty_				\
	 : len_ <= UCHAR_MAX						\
	 ? (qual_ GuString*) gu_string_short_(qual_,cstr_,len_)		\
	 : (qual_ GuString*) &gu_string_long_(qual_,cstr_,len_)->ss)

#define gu_string_(qual_,cstr_) \
	gu_string__(qual_,cstr_,(sizeof(cstr_)-1))

#define gu_string(cstr_)				\
	gu_string_(,cstr_)

#define gu_cstring(cstr_)				\
	gu_string_(const,cstr_)


inline int
gu_string_length(const GuString* s) {
	int len = ((const unsigned char*) s)[0];
	if (G_LIKELY(len > 0)) {
		return len;
	} else {
		return ((const int*) s)[-1];
	}
}

inline const char*
gu_string_cdata(const GuString* s) {
	return &((const char*) s)[1];
}

inline char*
gu_string_data(GuString* s) {
	return (char*) gu_string_cdata(s);
}




#endif // GU_STRING_H_
