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

#include <gu/exn.h>
#include <gu/mem.h>
#include <gu/in.h>
#include <gu/string.h>
#include <gu/enum.h>

typedef GuString PgfCId;
extern GU_DECLARE_TYPE(PgfCId, typedef);


/// A single lexical token			      
typedef GuString PgfToken;			      

/// @name PGF Grammar objects
/// @{

typedef struct PgfPGF PgfPGF;

/**< A representation of a PGF grammar. 
 */


typedef struct PgfConcr PgfConcr;

GuString
pgf_concr_id(PgfConcr* concr);
/**< Get the identifier of the concrete grammar. */

GuString
pgf_concr_lang(PgfConcr* concr);




GuEnum*
pgf_pgf_concrs(PgfPGF* pgf, GuPool* pool);

PgfConcr*
pgf_pgf_concr(PgfPGF* pgf, GuString cid, GuPool* pool);

PgfConcr*
pgf_pgf_concr_by_lang(PgfPGF* pgf, GuString lang, GuPool* pool);



typedef struct PgfCat PgfCat;
PgfCat*
pgf_pgf_cat(PgfPGF* pgf, PgfCId cid);


#include <gu/type.h>
GU_DECLARE_TYPE(PgfPGF, struct);

/// @}


#endif // PGF_H_
