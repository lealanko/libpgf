#ifndef LIBPGF_H_
#define LIBPGF_H_

/** @mainpage
 *
 * This is `libpgf`, a library for processing Portable Grammar Format grammars
 * that are produced by the [Grammatical Framework]
 * (http://grammaticalframework.org). This documentation assumes familiarity
 * with GF grammars.
 *
 * # Features and limitations
 * 
 * The library currently supports the following functionality:
 *
 * - reading a PGF grammar from a file
 * - incrementally parsing a sequence of tokens into abstract syntax trees (ASTs)
 * - creating and manipulating ASTs manually
 * - linearizing ASTs into sequences of tokens
 *
 * The currently supported class of grammars is heavily restricted. Grammars
 * may only contain category and token references. This means that the
 * following grammar features cannot be used:
 *
 * - literal float, string and int categories
 * - custom categories
 * - higher-order abstract syntax (lambdas and metavariables)
 *
 * Also, the following GF features are not yet supported:
 *
 * - generation of random sentences
 * - type checking
 *
 * # Quick tutorial trail
 *
 * The following sections should be enough to get you up to speed with the library.
 *
 * - Memory pools: gu/mem.h
 * - Exceptions: gu/exn.h
 * - PGF reading: pgf/reader.h
 * - Looking up concrete grammars from a PGF: pgf/pgf.h
 * - Parsing token streams: pgf/parser.h
 * - Representing abstract syntax trees: pgf/expr.h
 * - Linearizing abstract syntax trees: pgf/linearize.h
 * 
 */

#include <pgf/pgf.h>
#include <pgf/expr.h>
#include <pgf/reader.h>
#include <pgf/parser.h>
#include <pgf/linearize.h>

#endif // LIBPGF_H_
