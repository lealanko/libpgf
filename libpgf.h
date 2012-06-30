/** @file
 * A Portable Grammar Format library.
 *
 * This is `libpgf`, a library for processing natural language with the
 * Portable Grammar Format grammars produced by the [Grammatical Framework].
 *
 * [Grammatical Framework]: http://www.grammaticalframework.org/
 *
 * To use `libpgf` in a source file, just include this file, libpgf.h, which
 * in turn includes all the individual headers of `libpgf`. Alternatively, the
 * required individual headers can be included one by one.
 *
 * The library is based on the types and conventions of `libgu`, documented in
 * libgu.h. To familiarize yourself with `libpgf`, it is suggested that
 * you study the headers in the following order:
 *
 * - PGF reading: pgf/reader.h
 * - Looking up concrete grammars from a PGF: pgf/pgf.h
 * - Parsing token streams: pgf/parser.h
 * - Representing abstract syntax trees: pgf/expr.h
 * - Linearizing abstract syntax trees: pgf/linearize.h
 * 
 * @author Lauri Alanko <lealanko@ling.helsinki.fi>
 *
 * @copyright Copyright 2010-2012 University of Helsinki. Released under the
 * @ref LGPL3.
 * 
 */


#ifndef LIBPGF_H_
#define LIBPGF_H_

#include <pgf/pgf.h>
#include <pgf/expr.h>
#include <pgf/reader.h>
#include <pgf/parser.h>
#include <pgf/linearize.h>

#endif // LIBPGF_H_
