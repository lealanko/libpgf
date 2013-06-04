LIBPGF                                                      {#mainpage}
======

This is a preview release of `libpgf`, a native-code library for parsing
and linearization of the PGF grammars produced by the [Grammatical
Framework][GF]. The library is designed to be a lightweight, portable,
easily embeddable alternative to the Haskell-based PGF runtime that is
distributed with GF.

This release is not yet ready for production use: essential
functionality is still missing, the API is still likely to change, and
the documentation is incomplete. This release is primarily meant for
early adopters who are interested in using `libpgf`, and who wish to
contribute to its design.


Getting the library
-------------------

The library is currently available only as source packages. It is
released under the @ref LGPL3. The current version is 0.2.

- [libpgf-0.2.tar.gz]
- [libpgf-0.1.tar.gz]


Prerequisites
-------------

This is a self-contained library: only a C99-conformant C compiler is
needed. The code is mostly portable C, although it makes some very
general assumptions about the architecture (mostly regarding the
representation of addresses) that should hold on modern systems. Still,
the code has only been tested on Linux-x86(-64) so far. Reports of
porting problems on other platforms are appreciated.

Although the code "only" requires C99-conformance, it seems that many
compilers fail at it subtly. In particular:

- Clang does not currently support "extern inline" properly.
- Sun C 5.9 apparently has a bug in its treatment of sizeof on compound
  array literals.

As a consequence, these compilers cannot be used in the current state of
the code. Modern versions of GCC, on the other hand, seem to work fine.


Installing
----------

This is a standard GNU Autotools package: `./configure; make; make
install` should do the trick. Read the attached @ref INSTALL file for
generic installation instructions. There are currently no interesting
special configuration options.

Pkg-config configuration files for the library are also provided.


Features and limitations
------------------------

The library currently supports the following functionality:

- reading a PGF grammar from a file
- incrementally parsing a sequence of tokens into abstract syntax trees (ASTs)
- creating and manipulating ASTs manually
- linearizing ASTs into sequences of tokens

The currently supported class of grammars is heavily restricted. Grammars
may only contain category and token references. This means that the
following grammar features cannot be used:

- literal float, string and int categories
- custom categories
- higher-order abstract syntax (lambdas and metavariables)

Also, the following GF features are not yet supported:

- generation of random sentences
- type checking


Programs
--------

There are two small programs included. These are mainly for testing
purposes and for demonstrating how to use the library.

The `pgf2yaml` program simply reads a PGF file from the standard input and
dumps it to the standard output in [YAML] format.

The `pgf-translate` program translates sentences of one language in a PGF
grammar into another. It is invoked:

	pgf-translate PGF-FILE FROM TO

Where `PGF-FILE` is a PGF file, and `FROM` and `TO` are language codes
or names of concrete grammars within the PGF file. The program prompts
for a line containing a full sentence of the start category in the
source language, and displays the destination language linearizations of
all possible parses of that sentence. Run the program with no arguments
for information on some further options.


libgu
-----

Along with `libpgf` proper, this distribution includes `libgu`, a
general-purpose utility library that `libpgf` is based on. `libgu` is usable
independently of `libpgf`, and may eventually be split into a separate
package. Do give it a try if you are looking for a library to make C
programming less painful.


Documentation
-------------

Documentation is still fragmentary, but some of the most important
headers have documentation comments. If you have [Doxygen] installed,
`make doxygen-doc` will generate HTML documentation for the library.

The introductory documentation for `libgu` and `libpgf` can be found in
libgu.h and libpgf.h, respectively.

The sources in @ref pgf-translate have some comments which may also
clarify how to use the library.


Python bindings
---------------

There now exist Python bindings for the library. See the [PyGF home page] for details.


Feedback
--------

Please report bugs to the [GF bug tracker]. For general questions,
comments and suggestions on `libpgf`, write to the [GF mailing list].


For questions and comments that are related to the core `libgu` library,
but not to PGF, please write directly to the [author][Lauri Alanko].


Credits
-------

`libpgf` and `libgu` are developed by [Lauri Alanko]. The PGF format
and parsing algorithms were created by [Krasimir Angelov].


[Doxygen]:		http://doxygen.org/
[GF]:			http://www.grammaticalframework.org/
[GF bug tracker]:	http://code.google.com/p/grammatical-framework/issues/
[GF mailing list]:	https://groups.google.com/group/gf-dev
[Lauri Alanko]:		mailto:lealanko@ling.helsinki.fi
[libpgf-0.2.tar.gz]:	http://www.grammaticalframework.org/libpgf/libpgf-0.2.tar.gz
[libpgf-0.1.tar.gz]:	http://www.grammaticalframework.org/libpgf/libpgf-0.1.tar.gz
[LGPL3]:		http://www.gnu.org/licenses/lgpl.html
[Krasimir Angelov]:	http://www.cse.chalmers.se/~krasimir/
[PyGF home page]:	http://www.grammaticalframework.org/libpgf/pygf
[YAML]:			http://yaml.org/

@page pgf-translate pgf-translate.c
@include ./utils/pgf-translate.c

@page LGPL3 GNU Lesser General Public License, version 3
@verbinclude COPYING.LESSER

@page INSTALL
@verbinclude INSTALL
