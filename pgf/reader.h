#ifndef GU_READER_H_
#define GU_READER_H_

#include <pgf/pgf.h>

PgfPGF*
pgf_read_pgf(GuIn* in, GuPool* pool, GuExn* err); 

/**< Read a grammar from a PGF file.
 *
 * @param from  PGF input stream.
 * The stream must be positioned in the beginning of a binary
 * PGF representation. After a succesful invocation, the stream is
 * still open and positioned at the end of the representation.
 *
 * @param[out] err_out  Raised error.
 * If non-\c NULL, \c *err_out should be \c NULL. Then, upon
 * failure, \c *err_out is set to point to a newly allocated
 * error object, which the caller must free with #g_exn_free
 * or #g_exn_propagate.
 *
 * @return A new PGF object, or \c NULL upon failure. The returned
 * object must later be freed with #pgf_free.
 *
 */


#endif // GU_READER_H_
