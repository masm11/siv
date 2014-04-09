#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(jpeg.c)
AM_INIT_AUTOMAKE(siv-jpeg, 0.90)
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

#
AC_SIV_CHECK

#
AC_CHECK_HEADER(jpeglib.h,,[AC_MSG_ERROR(jpeglib.h not found)])
AC_CHECK_LIB(jpeg, jpeg_start_decompress,,[AC_MSG_ERROR(libjpeg not found.)])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
