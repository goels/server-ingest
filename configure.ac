#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([server-ingest], [1.0], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([remap/RemapIntf.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

AM_INIT_AUTOMAKE
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.28])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([rmdir strchr strerror strstr])

AC_CONFIG_FILES([Makefile
                 ifs/Makefile
                 remap/Makefile])
AC_OUTPUT
