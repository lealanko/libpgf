// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef PGF_PARSER_H_
#define PGF_PARSER_H_

#include <libgu.h>
#include <pgf/pgf.h>
#include <pgf/expr.h>

/// Parsing
/** @file
 *
 *  @todo Querying the parser for expected continuations
 *
 *  @todo Literals and custom categories
 *  
 *  @todo HOAS, dependent types...
 */

typedef struct PgfParse PgfParse;

/** @name Creating a new parser
 *
 * A #PgfParser object can parse sentences of a single concrete category into
 * abstract syntax trees (#PgfExpr). The parser is created with
 * #pgf_new_parser.
 *
 * @{ 
 */

/// A parser for a single concrete category
typedef struct PgfParser PgfParser;
    

/// Create a new parser
PgfParser*
pgf_new_parser(PgfConcr* concr, GuPool* pool);
/**<
 * @param concr The concrete grammar whose sentences are to be parsed
 *
 * @pool
 *
 * @return A newly created parser for the concrete category `concr`
 */ 

/** @}
 * 
 * @name Parsing a sentence
 *
 * The progress of parsing is controlled by the client code. Firstly, the
 * parsing of a sentence is initiated with #pgf_parser_parse. This returns an
 * initial #PgfParse object, which represents the state of the parsing. A new
 * parse state is obtained by feeding a token with #pgf_parse_token. The old
 * parse state is unaffected by this, so backtracking - and even branching -
 * can be accomplished by retaining the earlier #PgfParse objects.
 *
 * @{
 */

/// Begin parsing
PgfParse*
pgf_parser_parse(PgfParser* parser, PgfCat* cat, PgfCtntId ctnt,
		 GuPool* pool);
/**<
 * @param parser The parser to use
 *
 * @param cat The abstract category to parse. This must be from the same
 * #PgfPGF as the concrete category used to create the parser.
 *
 * @param ctnt The index of the constituent in the concrete category to parse.
 * Given a label, this can be looked up with #pgf_concr_cat_ctnt_id.
 *
 * @pool
 *
 * @return An initial parsing state.
*/


/// Feed a token to the parser
PgfParse*
pgf_parse_token(PgfParse* parse, PgfToken tok, GuPool* pool);
/**<
 * @param parse The current parse state
 *
 * @param tok The token to feed
 *
 * @pool
 *
 * @return A new parse state obtained by feeding `tok` as an input to 
 * `parse`, or `NULL` if the token was unexpected.
 *
 * @note The new parse state partially depends on the old one, so it doesn't
 * make sense to use a `pool` argument with a longer lifetime than that of
 * the pool used to create `parse`.
 */


/** @}
 * @name Retrieving abstract syntax trees
 *
 * After the desired tokens have been fed to the parser, the resulting parse
 * state can be queried for completed results. The #pgf_parse_result function
 * returns an enumeration (#GuEnum) of possible abstract syntax trees whose
 * linearization is the sequence of tokens fed so far.
 *
 * @{
 */


/// An enumeration of #PgfExpr elements.
typedef GuEnum PgfExprEnum;


/// Retrieve the current parses from the parse state.
PgfExprEnum*
pgf_parse_result(PgfParse* parse, GuPool* pool);
/**<
 * @param parse A parse state
 *
 * @pool
 * 
 * @return An enumeration of #PgfExpr elements representing the abstract
 * syntax trees that would linearize to the sequence of tokens fed to produce
 * \p parse. The enumeration may yield zero, one or more abstract syntax
 * trees, depending on whether the parse was unsuccesful, unambiguously
 * succesful, or ambiguously successful.
 */


/** @} */

#endif // PGF_PARSER_H_
