#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([joystck_powerlink_cn], [0.1], [bugs@noop.org])
AM_INIT_AUTOMAKE([-Wall, -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
PKG_PROG_PKG_CONFIG

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_mutex_init])
AC_CHECK_LIB([rt], [timer_create])
AC_CHECK_LIB([oplkcn], [oplk_initialize])

# Checks for modules:
PKG_CHECK_MODULES([NCURSES],[ncurses])
PKG_CHECK_MODULES([PCAP],[libpcap])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([memset select strerror strtoul])

# Powerlink defines:
#AC_DEFINE([CONFIG_KERNELSTACK_DIRECTLINK],[],[])
AC_DEFINE([CONFIG_USE_SYNCTHREAD],[],[Create separate thread for syncronous data exchange])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
