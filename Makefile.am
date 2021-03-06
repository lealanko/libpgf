LDADD = libgu.la

lib_LTLIBRARIES = libgu.la libpgf.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgu.pc libpgf.pc

configincludedir = $(libdir)/libgu/include
nodist_configinclude_HEADERS = guconfig.h

guincludedir=$(includedir)/gu
guinclude_HEADERS = \
	gu/assert.h \
	gu/bits.h \
	gu/choice.h \
	gu/defs.h \
	gu/dump.h \
	gu/enum.h \
	gu/exn.h \
	gu/file.h \
	gu/fun.h \
	gu/generic.h \
	gu/hash.h \
	gu/in.h \
	gu/intern.h \
	gu/list.h \
	gu/log.h \
	gu/map.h \
	gu/mem.h \
	gu/out.h \
	gu/prime.h \
	gu/print.h \
	gu/read.h \
	gu/seq.h \
	gu/str.h \
	gu/string.h \
	gu/sysdeps.h \
	gu/type.h \
	gu/ucs.h \
	gu/utf8.h \
	gu/variant.h \
	gu/write.h \
	gu/yaml.h \
	libgu.h

libgu_la_SOURCES = \
	gu/assert.c \
	gu/bits.c \
	gu/choice.c \
	gu/defs.c \
	gu/seq.c \
	gu/dump.c \
	gu/enum.c \
	gu/exn.c \
	gu/file.c \
	gu/fun.c \
	gu/generic.c \
	gu/hash.c \
	gu/in.c \
	gu/intern.c \
	gu/list.c \
	gu/log.c \
	gu/map.c \
	gu/mem.c \
	gu/out.c \
	gu/prime.c \
	gu/print.c \
	gu/read.c \
	gu/str.c \
	gu/string.c \
	gu/type.c \
	gu/utf8.c \
	gu/write.c \
	gu/ucs.c \
	gu/variant.c \
	gu/yaml.c

pgfincludedir=$(includedir)/pgf
pgfinclude_HEADERS = \
	pgf/expr.h \
	pgf/linearize.h \
	pgf/parser.h \
	pgf/pgf.h \
	pgf/reader.h \
	libpgf.h

libpgf_la_SOURCES = \
	pgf/data.c \
	pgf/data.h \
	pgf/edsl.h \
	pgf/expr.c \
	pgf/expr.h \
	pgf/parser.c \
	pgf/parser.h \
	pgf/pgf.c \
	pgf/reader.c \
	pgf/linearize.c

if BUILD_PGF_TRANSLATE

PGF_TRANSLATE = utils/pgf-translate
utils_pgf_translate_SOURCES = utils/pgf-translate.c
utils_pgf_translate_LDADD = libpgf.la libgu.la

endif

bin_PROGRAMS = \
	utils/pgf2yaml \
	$(PGF_TRANSLATE)

utils_pgf2yaml_SOURCES = utils/pgf2yaml.c
utils_pgf2yaml_LDADD = libpgf.la libgu.la 


noinst_PROGRAMS = \
	test/test-write

test_test_write_SOURCES = test/test-write.c
test_test_write_LDADD = libgu.la

AUTOMAKE_OPTIONS = foreign subdir-objects dist-bzip2
ACLOCAL_AMFLAGS = -I m4
include doxygen.am

EXTRA_DIST = \
	INSTALL \
	Doxyfile \
	DoxygenLayout.xml \
	libgu.pc.in \
	libpgf.pc.in \
	guconfig.h.in
