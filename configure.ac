#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([fonulator], 2.0.2, bcarrington@red-fone.com)
AC_DEFINE_UNQUOTED([FONULATOR_VERSION], ["2.0.2"], [Official Version Number])
AC_CONFIG_SRCDIR([state.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX

# Checks for libraries.
AC_CHECK_LIB([fl], [yylex_destroy])

# Enable `--with-shared-libfb''
AC_ARG_WITH([with-shared-libfb],
            [AS_HELP_STRING([--with-shared-libfb], [use shared libfb instead of static version])],
            [with_shared_libfb=yes],
            [])

# We link libargtable2 in statically using Automake
AC_CHECK_FILE(/usr/lib/libargtable2.a,	
              [AC_DEFINE(HAVE_STATIC_LIBARGTABLE, [], [Define to 1 if you have the static version of argtable in the right place.])],
	      [AC_MSG_ERROR([Couldn't find static argtable2 libraries])])

# Optional shared linking with libfb
LIBFB=
 AS_IF([test "x$with_shared_libfb" = xyes],
	AC_CHECK_LIB([fb], [libfb_init],
		     [AC_SUBST([LIBFB], ["-lfb"])
		     AC_DEFINE([HAVE_LIBFB], [1], [Define to 1 if you have libfb])
		     ],
		     AC_MSG_FAILURE([test for libfb failed (required)])),
		     # otherwise, we need to link static libfb and also link in libnet and libpcap
	# Check for static libfb
	AC_CHECK_FILE(/usr/local/lib/libfb.a,
	      [
	      AC_SUBST([LIBFB], ["/usr/local/lib/libfb.a"])
	      AC_DEFINE(HAVE_STATIC_LIBFB, 1, [Define to 1 if you have the static version of libfb in the right place.])
	      ],
	      [AC_MSG_ERROR([Couldn't find static libfb libraries])])
	# Check for shared libnet
	AC_CHECK_LIB([net], [libnet_write],
		     [AC_SUBST([LIBNET], ["-lnet"])
		     AC_DEFINE([HAVE_LIBNET], [1], [Define if you have libnet])
		     ],
		     AC_MSG_FAILURE([test for libnet failed (required)]))
	# Check for shared libpcap
	AC_CHECK_LIB([pcap], [pcap_next],
		     [AC_SUBST([LIBPCAP], ["-lpcap"])
		     AC_DEFINE([HAVE_LIBPCAP], [1], [Define if you have libpcap])
		     ],
		     AC_MSG_FAILURE([test for libpcap failed (required)]))
    )

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([netdb.h stdbool.h stdio.h stdlib.h unistd.h])

# Library headers
AC_CHECK_HEADERS([argtable2.h libnet.h pcap.h libfb/fb_lib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UINT32_T
AC_TYPE_UINT16_T
AC_TYPE_INT32_T
AC_TYPE_INT16_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyaddr gethostbyname memset socket strchr])
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
