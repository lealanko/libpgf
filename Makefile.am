
LDADD = libgu.la

lib_LTLIBRARIES = libgu.la libpgf.la

include_HEADERS = \
	pgf/pgf.h \
	gu/bits.h \
	gu/choice.h \
	gu/defs.h \
	gu/error.h \
	gu/seq.h \
	gu/dump.h \
	gu/error.h \
	gu/intern.h \
	gu/list.h \
	gu/log.h \
	gu/map.h \
	gu/mem.h \
	gu/prime.h \
	gu/string.h \
	gu/type.h \
	gu/variant.h \
	gu/yaml.h

libgu_la_SOURCES = \
	gu/bits.c \
	gu/choice.c \
	gu/seq.c \
	gu/dump.c \
	gu/error.c \
	gu/fun.c \
	gu/intern.c \
	gu/list.c \
	gu/log.c \
	gu/map.c \
	gu/mem.c \
	gu/prime.c \
	gu/string.c \
	gu/type.c \
	gu/variant.c \
	gu/yaml.c

libpgf_la_SOURCES = \
	pgf/data.c \
	pgf/data.h \
	pgf/edsl.h \
	pgf/expr.c \
	pgf/expr.h \
	pgf/reader.c \
	pgf/linearize.c

libpgf_la_LIBADD = libgu.la $(GLIB_LIBS)
libpgf_la_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)

bin_PROGRAMS = utils/pgf2yaml
utils_pgf2yaml_SOURCES = utils/pgf2yaml.c
utils_pgf2yaml_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)
utils_pgf2yaml_LDADD = libgu.la libpgf.la $(GLIB_LIBS)

noinst_PROGRAMS = \
	test/test_type1 \
	test/test-hash \
	test/test-lin1 \
	test/test-yaml1 \
	test/test-dump1 \
	test/test-seq \
	test/test-prime


test_test_type1_SOURCES = test/test_type1.c
test_test_lin1_SOURCES = test/test-lin1.c
test_test_lin1_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)
test_test_lin1_LDADD = libgu.la $(GLIB_LIBS) libpgf.la 
test_test_yaml1_SOURCES = test/test-yaml1.c
test_test_dump1_SOURCES = test/test-dump1.c
test_test_seq_SOURCES = test/test-seq.c
test_test_hash_SOURCES = test/test-hash.c
test_test_prime_SOURCES = test/test-prime.c



AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
include doxygen.am

EXTRA_DIST = Doxyfile