// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

/** @file
 *
 * PGF Grammars.
 */

#ifndef PGF_H_
#define PGF_H_

#include <gu/exn.h>
#include <gu/mem.h>
#include <gu/in.h>
#include <gu/seq.h>
#include <gu/string.h>
#include <gu/enum.h>



/** @name PGF Grammar objects
 */

/// A PGF grammar.
typedef struct PgfPGF PgfPGF;


/** @name Identifiers
 *
 * Identifiers are used to name different parts of PGF grammars: concrete
 * grammars, abstract categories, and abstract functions.
 * 
 */


/// A PGF identifier.
typedef GuString PgfCId;

/**< Each identifier is a #GuString that conforms to the lexical constraints
 * of PGF identifiers.
 */

extern GU_DECLARE_TYPE(PgfCId, typedef);



/** @name Abstract categories.
 *
 *  
 * 
 */

/// An abstract category.
typedef struct PgfCat PgfCat;

PgfCat*
pgf_pgf_startcat(PgfPGF* pgf);

PgfCat*
pgf_pgf_cat(PgfPGF* pgf, PgfCId cid);



/** @name Concrete grammars
 *
 * Each PGF grammar contains a number of 
 */

typedef struct PgfConcr PgfConcr;

GuString
pgf_concr_id(PgfConcr* concr);
/**< Get the identifier of the concrete grammar. */

GuString
pgf_concr_lang(PgfConcr* concr);

GuStrings
pgf_concr_cat_labels(PgfConcr* concr, PgfCat* cat, GuPool* pool);




GuEnum*
pgf_pgf_concrs(PgfPGF* pgf, GuPool* pool);

PgfConcr*
pgf_pgf_concr(PgfPGF* pgf, GuString cid, GuPool* pool);

PgfConcr*
pgf_pgf_concr_by_lang(PgfPGF* pgf, GuString lang, GuPool* pool);




/// A single lexical token			      
typedef GuString PgfToken;			      



#include <gu/type.h>
GU_DECLARE_TYPE(PgfPGF, referenced);


#endif // PGF_H_
