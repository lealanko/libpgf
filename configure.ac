AC_INIT(Portable Grammar Format library, 0.0, lealanko@ling.helsinki.fi, libpgf)
AC_PREREQ(2.58)

AC_CONFIG_SRCDIR([gu/mem.c])
AC_CONFIG_AUX_DIR([scripts])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.7.9])

AC_CONFIG_HEADERS([config.h guconfig.h])

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
DOLT


dnl Doxygen support

DX_PS_FEATURE(OFF)	
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


dnl Check if pkg-config is installed
AC_CHECK_PROG(PKGCONFIG, pkg-config, yes, no)
if test $PKGCONFIG = "no"; then
	AC_MSG_ERROR("*** pkg-config not found! ***")
fi

AC_CHECK_FUNCS([alarm gettimeofday memset strndup inet_ntoa])
AC_CHECK_FUNCS([strstr select strerror])
AC_C_ALIGNOF
AC_C_FAM_IN_MEM

if test $ac_cv_c_alignof != no; then
  AC_DEFINE_UNQUOTED([gu_alignof], [$ac_cv_c_alignof], [foo])
fi
if test $ac_cv_c_fam_in_mem != no; then
  AC_DEFINE([GU_CAN_HAVE_FAM_IN_MEMBER], 1, [foo])
fi









AC_CHECK_LIB(m, sin)
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.24.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
PKG_CHECK_MODULES(GIO, [gio-2.0 >= 2.24.0])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)



AC_CONFIG_FILES([Makefile
	         libgu.pc
		 ])

AC_OUTPUT
