AC_INIT([libmarquise], [0.0.1], [pingu@anchor.net.au])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT

AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

PKG_CHECK_MODULES([GLIB_2], [glib-2.0 >= 2.5])
# Debian bug #731726, :(, hard coded for now.
# PKG_CHECK_MODULES([PROTOBUF_C], [libprotobuf-c >= 0.1])
AC_SUBST(PROTOBUF_LIBS)
AC_SUBST(PROTOBUF_CFLAGS)
AC_SUBST(PROTOBUF_VERSION)

AC_CHECK_PROG([PROTOC_C], [protoc-c], [protoc-c])
AS_IF([test "x${PROTOC-C}" == "x"],
          [AC_MSG_ERROR([ProtoBuf compiler "protoc-c" not found.])])

AC_CHECK_LIB([zmq], [zmq_connect])

AC_CHECK_HEADERS([stdint.h stdlib.h string.h syslog.h unistd.h])

AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

AC_CHECK_FUNCS([strerror])

AC_CONFIG_HEADERS([config.h])
AC_CHECK_HEADERS([stdlib.h string.h syslog.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
