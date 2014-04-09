#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(src/image.c)
AM_INIT_AUTOMAKE(siv, 1.97)
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_ARG_ENABLE([decoder-process],
  AS_HELP_STRING([--enable-decoder-process],
		 [use decoder processes. (default: disable)]),
  AC_DEFINE(DECODER_PROCESS, 1, [Define to 1 if you use decoder processes.])
  AC_CHECK_LIB([rt], [shm_open]))

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([ftruncate getcwd memset munmap])

PKG_CHECK_MODULES(GSTUFF, gtk+-2.0 >= 2.4.0 glib-2.0 >= 2.4.0 gobject-2.0 >= 2.4.0 gthread-2.0 >= 2.4.0)

#
AC_SUBST(pkglibexecdir, '$(libexecdir)/siv')

# 
AC_MSG_CHECKING([for aclocaldir])
AC_ARG_ENABLE(honor-aclocaldir,
AS_HELP_STRING([--disable-honor-aclocaldir],
 [disable for distcheck.]),
[honor_aclocaldir=${enableval}],
[honor_aclocaldir=yes])
case "${honor_aclocaldir}" in
yes) aclocaldir="`aclocal --print-ac-dir`";;
no)  aclocaldir='$(datadir)/aclocal';;
*)   AC_MSG_ERROR([bad honor-aclocaldir]);;
esac
AC_SUBST(aclocaldir, "${aclocaldir}")
AC_MSG_RESULT(${aclocaldir})

AC_CONFIG_FILES([Makefile
   src/Makefile
   src/images/Makefile
   include/Makefile
   include/siv/Makefile])
AC_OUTPUT
