/** @file
 * A general utility library.
 *
 * This is `libgu`, a general-purpose utility library for making C programming
 * bearable.
 * 
 * To use `libgu` in a source file, just include this file, libgu.h, which
 * in turn includes all the individual headers of `libgu`. Alternatively, the
 * required individual headers can be included one by one.
 *
 * The library is not yet well documented, but the following headers explain
 * some core concepts:
 *
 * - Memory management with memory pools: gu/mem.h
 * - Error notification and handling: gu/exn.h
 * 
 * @author Lauri Alanko <lealanko@helsinki.fi>
 *
 * @copyright Copyright 2010-2012 University of Helsinki. Released under the
 * @ref LGPL3.
 *
 */

#ifndef LIBGU_H_
#define LIBGU_H_

#include <gu/defs.h>
#include <gu/mem.h>
#include <gu/type.h>
#include <gu/exn.h>
#include <gu/seq.h>
#include <gu/map.h>
#include <gu/ucs.h>
#include <gu/utf8.h>
#include <gu/string.h>
#include <gu/in.h>
#include <gu/out.h>
#include <gu/file.h>
#include <gu/read.h>
#include <gu/write.h>
#include <gu/variant.h>
#include <gu/intern.h>
#include <gu/prime.h>

#endif // LIBGU_H_
