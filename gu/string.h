// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef GU_STRING_H_
#define GU_STRING_H_

#include <gu/bits.h>
#include <gu/read.h>
#include <gu/write.h>

/** @file
 *
 * Strings.
 */



/** @name Basics
 */


/// A string.
typedef GU_OPAQUE GuString;

/**< A #GuString represents a sequence of Unicode codepoints. The
 * representation is likely to be more efficient than using simply an array of
 * #GuUCS values.
 *
 * A #GuString is immutable: once created, its value cannot be changed. 
 */


/// An empty string.
extern const GuString gu_empty_string;

extern const GuString gu_null_string;

/// Create a string from a C string.
GuString
gu_str_string(const char* str, GuPool* pool);


/// Create a string from a sequence of UCS code points.
GuString
gu_ucs_string(const GuUCS* ubuf, size_t len, GuPool* pool);

/// Create a string from a UTF-8 encoded buffer.
GuString
gu_utf8_string(const uint8_t* buf, size_t sz, GuPool* pool);


/// Copy a string.
GuString
gu_string_copy(GuString string, GuPool* pool);

/**<
 * @return a new string, representing the same code point sequence as `string`,
 * but allocated from `pool`.
 *
 * @note Since strings are immutable, the only situation where this function
 * is needed is when `string` has the desired value, but too short lifetime.
 */


/// Compare two strings for equality.
bool
gu_string_eq(GuString s1, GuString s2);

/**< @return `true` iff `s1` and `s2` represent the same sequences of code points.
 */

bool
gu_string_is_null(GuString s);




/** @name Formatting
 */

GuString
gu_format_string_v(const char* fmt, va_list args, GuPool* pool);

GuString
gu_format_string(GuPool* pool, const char* fmt, ...);





/** @name I/O
 */

void
gu_string_write(GuString string, GuWriter* wtr, GuExn* err);

GuReader*
gu_string_reader(GuString string, GuPool* pool);

/** @name String buffers
 */

typedef struct GuStringBuf GuStringBuf;

GuStringBuf*
gu_string_buf(GuPool* pool);

GuWriter*
gu_string_buf_writer(GuStringBuf* sb);

GuString
gu_string_buf_freeze(GuStringBuf* sb, GuPool* pool);




/** @name Low-level operations
 */

bool
gu_string_is_stable(GuString string);




#endif // GU_STRING_H_

/** @name Miscellaneous
 */

#if defined(GU_HASH_H_) && !defined(GU_STRING_H_HASH_)
#define GU_STRING_H_HASH_

uintptr_t
gu_string_hash(GuString s);

extern GuHasher gu_string_hasher[1];

#endif

#ifdef GU_TYPE_H_
# ifndef GU_STRING_H_TYPE_
#  define GU_STRING_H_TYPE_

extern GU_DECLARE_TYPE(GuString, GuOpaque);
# endif

# if defined(GU_SEQ_H_) && !defined(GU_STRING_H_SEQ_TYPE_)
#  define GU_STRING_H_SEQ_TYPE_
extern GU_DECLARE_TYPE(GuStrings, GuSeq);
# endif

# if defined(GU_MAP_H_TYPE_) && !defined(GU_STRING_H_MAP_TYPE_)
#  define GU_STRING_H_MAP_TYPE_

extern GU_DECLARE_KIND(GuStringMap);
typedef GuType_GuMap GuType_GuStringMap;

#define GU_TYPE_INIT_GuStringMap(KIND, MAP_T, VAL_T, DEFAULT)	\
	GU_TYPE_INIT_GuMap(KIND, MAP_T,				\
			   gu_type(GuString), gu_string_hasher,	\
			   VAL_T, DEFAULT)

# endif
#endif


#if defined(GU_SEQ_H_) && !defined(GU_STRING_H_SEQ_)
#define GU_STRING_H_SEQ_

typedef GuSeq GuStrings;
// typedef GuBuf GuStringBuf;

#endif


#if defined(GU_MAP_H_) && !defined(GU_STRING_H_MAP_)
#define GU_STRING_H_MAP_

typedef GuMap GuStringMap;

#define gu_new_string_map(VAL_T, DEFAULT, POOL)				\
	gu_new_map(GuString, gu_string_hasher, (VAL_T), (DEFAULT), (POOL))

#endif

