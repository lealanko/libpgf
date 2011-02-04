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

/** @file
 *
 * The public libpgf API.
 */

#ifndef PGF_H_
#define PGF_H_

#include <glib.h>
#include <stdio.h>

G_BEGIN_DECLS



/// @name PGF Grammar objects
/// @{

typedef struct PgfPGF PgfPGF;

/**< A representation of a PGF grammar. 
 */


PgfPGF* pgf_read(FILE* from, GError** err_out); 

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
 * error object, which the caller must free with #g_error_free
 * or #g_error_propagate.
 *
 * @return A new PGF object, or \c NULL upon failure. The returned
 * object must later be freed with #pgf_free.
 *
 */


void pgf_write_yaml(PgfPGF* pgf, FILE* to, GError** err_out);

/**< Write out a grammar as YAML.
 *
 * This function writes out the structure of the PGF grammar in the
 * "flow" style of the human- and machine-readable <a
 * href="http://yaml.org/">YAML</a> format. The output is not nicely
 * formatted, but can be further processed by other YAML tools to
 * produce e.g. indented block-style YAML output.
 *
 * @param pgf  The grammar to write out.
 *
 * @param to   The YAML output stream.
 * After a succesful invocation, the stream remains open.
 *
 * @param[out] err_out  Raised error.
 * If non-\c NULL, \c *err_out should be \c NULL. Then, upon
 * failure, \c *err_out is set to point to a newly allocated
 * error object, which the caller must free with #g_error_free
 * or #g_error_propagate.
 *
 */


void pgf_free(PgfPGF* pgf);
/**< Free a grammar object.
 *
 * @param pgf  Grammar object to free. After invocation, it must not be
 * used any more.
 */

/// @}


#include <gu/type.h>
GU_DECLARE_TYPE(PgfPGF, struct);

G_END_DECLS

#endif // PGF_H_
