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

typedef struct GuString GuString;

typedef const GuString GuCString;

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



// internal

#include <limits.h>

#define GuStringShortN_(cstr_)				\
	struct {					\
		unsigned char len;			\
		char data[sizeof(cstr_)-1];		\
	}
#define GuStringLongN_(cstr_)				\
	struct {					\
		int true_len;				\
		GuStringShortN_(cstr_) ss;		\
	}


#define GU_STRING_SHORTN_INIT_(cstr_) {				\
		.len = (unsigned char)(sizeof(cstr_)-1),	\
		.data = cstr_,					\
		}

#define gu_string_short_(qual_,cstr_)			\
	((qual_ GuStringShortN_(cstr_)[1]) {		\
	GU_STRING_SHORTN_INIT_(cstr_)			\
	})

#define GU_STRING_LONGN_INIT_(cstr_) {			\
		.true_len = (sizeof(cstr_)-1),		\
		.ss = GU_STRING_SHORTN_INIT_(cstr_),	\
		}

#define gu_string_long_(qual_,cstr_)			\
	((qual_ GuStringLongN_(cstr_)[1]) {	\
		GU_STRING_LONGN_INIT_(cstr_)		\
		})

#define GU_DEFINE_ATOM(name_, cstr_)		\
	const GuStringShortN_(cstr_)	\
	gu_atom_##name_##_ =			\
	GU_STRING_SHORTN_INIT_(cstr_)

#define gu_atom(name_)				\
	((const GuString*)&gu_atom_##name_##_)

#define gu_string_(qual_,cstr_)					\
	((sizeof(cstr_)-1 > 0 && sizeof(cstr_)-1 <= UCHAR_MAX)	\
	 ? (qual_ GuString*) gu_string_short_(qual_,cstr_)	\
	 : (qual_ GuString*) &gu_string_long_(qual_,cstr_)->ss)

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


#include <gu/list.h>
typedef GuList(GuString*) GuStrings;
typedef GuList(GuCString*) GuCStrings;
		            		      


#endif // GU_STRING_H_
