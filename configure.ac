AC_INIT(Portable Grammar Format library, 0.0, lealanko@ling.helsinki.fi, libpgf)
AC_PREREQ(2.58)

AC_CONFIG_SRCDIR([gu/mem.c])
AC_CONFIG_AUX_DIR([scripts])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.7.9])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_CONFIG_HEADERS([config.h])

#AC_GNU_SOURCE
#AC_CANONICAL_HOST
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CC_C99

# we use DOLT, a libtool wrapper (speed improvement)
# see m4/dolt.m4
# DOLT


AC_CHECK_FUNCS([alarm gettimeofday memset strndup inet_ntoa])
AC_CHECK_FUNCS([strstr select strerror])
AC_C_ALIGNOF
AC_C_FAM_IN_MEM
AC_C_STATEMENT_EXPRESSIONS
AC_C_ASCII


AC_CHECK_LIB(m, sin)

dnl Doxygen support
DX_PS_FEATURE(OFF)	
DX_PDF_FEATURE(OFF)	
AC_DEFUN([DX_FEATURE_dev],   OFF)
DX_ARG_ABLE(dev, [include internal development documentation],
            [],
            [],
	    [],
	    [DX_ENV_APPEND(DEVDOC, YES)]
	    [DX_ENV_APPEND(INPUT, $srcdir)],
	    [DX_ENV_APPEND(DEVDOC, NO)
	     DX_ENV_APPEND(INPUT, \$(include_HEADERS))])
DX_INIT_DOXYGEN(libpgf)


AC_CONFIG_FILES([Makefile
	         libgu.pc
		 ])

AC_OUTPUT
