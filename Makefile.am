
lib_LTLIBRARIES = libgu.la libpgf.la

include_HEADERS = \
	pgf/pgf.h \
	gu/macros.h \
	gu/flex.h \
	gu/map.h \
	gu/mem.h \
	gu/variant.h

libgu_la_SOURCES = \
	gu/flex.c \
	gu/map.c \
	gu/mem.c \
	gu/variant.c

libgu_la_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)
libgu_la_LIBADD = $(GLIB_LIBS)

libpgf_la_SOURCES = \
	pgf/data.h \
	pgf/reader.c \
	pgf/linearize.c

libpgf_la_LIBADD = libgu.la $(GLIB_LIBS)
libpgf_la_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)

bin_PROGRAMS = pgf2yaml
pgf2yaml_SOURCES = utils/pgf2yaml.c
pgf2yaml_LDADD = libgu.la libpgf.la $(GLIB_LIBS)
pgf2yaml_CFLAGS = $(GLIB_CFLAGS) $(AM_CFLAGS)


AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
include doxygen.am

EXTRA_DIST = Doxyfile