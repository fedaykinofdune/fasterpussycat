#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([fasterpussycat], [0.01], [dirtyfilthe@gmail.com])
AM_INIT_AUTOMAKE(fasterpussycat, 0.1)
AC_CONFIG_SRCDIR([src/shoggoth/http_response.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
ZMQ_REQUIRED_VERSION=0.9
SSL_REQUIRED_VERSION=0.9
PKG_CHECK_MODULES(ZMQ, libzmq >= $ZMQ_REQUIRED_VERSION)

AC_CHECK_LIB([z],[gzread],[Z_LIBS=-lz])
AC_CHECK_HEADER(zlib.h,[],AC_MSG_ERROR([failed to find the required header file zlib.h]))
AC_SUBST([Z_LIBS])
PKG_CHECK_MODULES(SSL, libssl >= $SSL_REQUIRED_VERSION)


# Checks for header files.
AC_CHECK_HEADERS([inttypes.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h])
AC_CHECK_HEADER(openssl/ssl.h,[],AC_MSG_ERROR([failed to find the required header file openssl/ssl.h]))
AC_CHECK_HEADER(openssl/err.h,[],AC_MSG_ERROR([failed to find the required header file openssl/err.h]))
AC_CHECK_HEADER(openssl/md5.h,[],AC_MSG_ERROR([failed to find the required header file openssl/md5.h]))
AC_CHECK_HEADER(lua5.1/lua.h,[],AC_MSG_ERROR([failed to find the required header file lua.h]))
  
AC_SEARCH_LIBS( [lua_pcall], [lua lua5.1])
AC_SUBST([LUA_LIBS])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostbyname memchr memset socket strcasecmp strdup strncasecmp])




AC_OUTPUT(Makefile src/Makefile src/fasterpussycat/Makefile src/shoggoth/Makefile)
