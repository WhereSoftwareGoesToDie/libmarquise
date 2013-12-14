AC_INIT([libanchor_stats], [0.0.1], [pingu@anchor.net.au])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
LT_INIT

AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_CHECK_LIB([zmq], [zmq_connect])

AC_CHECK_HEADERS([stdlib.h string.h syslog.h unistd.h])

AC_TYPE_SIZE_T

AC_CHECK_FUNCS([strerror])

AC_CONFIG_HEADERS([config.h])
AC_CHECK_HEADERS([stdlib.h string.h syslog.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
