// Copyright -2012 University of Helsinki. Released under LGPL3.

#ifndef GU_READER_H_
#define GU_READER_H_

#include <pgf/pgf.h>

/** @file
 * Reading PGF grammars.
 */

PgfPGF*
pgf_read_pgf(GuIn* in, GuPool* pool, GuExn* exn); 

/**< Read a grammar from a PGF file.
 *
 * @param in  PGF input stream.
 * The stream must be positioned in the beginning of a binary
 * PGF representation. After a succesful invocation, the stream is
 * still open and positioned at the end of the representation.
 *
 * @param pool  The pool to allocate from.
 *
 * @param[out] exn  Current exception frame.
 *
 * @return A new PGF object allocated from `pool`, or `NULL` upon failure.
 */


#endif // GU_READER_H_
