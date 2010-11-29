
lib_LTLIBRARIES = libgu.la libpgf.la

include_HEADERS = \
	pgf/pgf.h \
	gu/defs.h \
	gu/flex.h \
	gu/map.h \
	gu/mem.h \
	gu/type.h \
	gu/variant.h

libgu_la_SOURCES = \
	gu/flex.c \
	gu/map.c \
	gu/mem.c \
	gu/type.c \
	gu/variant.c

libgu_la_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)
libgu_la_LIBADD = $(GLIB_LIBS)

libpgf_la_SOURCES = \
	pgf/data.h \
	pgf/reader.c \
	pgf/linearize.c

libpgf_la_LIBADD = libgu.la $(GLIB_LIBS)
libpgf_la_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)

bin_PROGRAMS = utils/pgf2yaml
utils_pgf2yaml_SOURCES = utils/pgf2yaml.c
utils_pgf2yaml_LDADD = libgu.la libpgf.la $(GLIB_LIBS)
utils_pgf2yaml_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)

noinst_PROGRAMS = test/test_type1
test_test_type1_SOURCES = test/test_type1.c
test_test_type1_LDADD = libgu.la
test_test_type1_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)


AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
include doxygen.am

EXTRA_DIST = Doxyfile