#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.62)
AC_INIT(pdfreflow, 0.8.6, blissenergy@gmail.com)
AM_INIT_AUTOMAKE(pdfreflow, 0.8.6)
AC_CONFIG_SRCDIR([src/attr.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset strcasecmp strchr strdup strrchr strstr strtol])

AC_OUTPUT(Makefile src/Makefile)
