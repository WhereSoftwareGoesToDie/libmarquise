AC_INIT([libmarquise], [2.1.2], [engineering@anchor.net.au])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT

AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S

PKG_CHECK_MODULES([GLIB_2], [glib-2.0 >= 2.5])

AC_CHECK_HEADERS([stdint.h stdlib.h string.h syslog.h unistd.h])

AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

AC_CHECK_FUNCS([strerror])

AC_CONFIG_HEADERS([config.h])
AC_CHECK_HEADERS([stdlib.h string.h syslog.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 doc/man/Makefile
])
AC_OUTPUT
