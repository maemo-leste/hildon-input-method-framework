# This file is part of hildon-input-method-framework
#
# Copyright (C) 2005-2007 Nokia Corporation.
#
# Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
#
#

AC_INIT(hildon-input-method-framework, 2.1.44)
AC_CONFIG_SRCDIR(Makefile.am)
AM_INIT_AUTOMAKE

LIBVERSION=3
AC_SUBST(LIBVERSION)

AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],
		[Debug (default=no)])],
		[hildon_use_debug=yes],[hildon_use_debug=no])

AC_ARG_ENABLE(instrumenting, [AC_HELP_STRING([--enable-instrumenting],
		[Compile with instrumentation flags (default=no)])],
		[hildon_use_instrumenting=yes],[hildon_use_instrumenting=no])


AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

GTK_DOC_CHECK(1.9)

if test foobar${hildon_use_debug} = foobaryes
then
	CFLAGS="$CFLAGS -DDEBUG"
fi

AM_CONDITIONAL(TIMER_ENABLED, test x${hildon_use_timestamping} = xyes)

CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Wno-deprecated-declarations"

PKG_CHECK_MODULES(GTK2, gtk+-2.0 hildon-1)
AC_SUBST(GTK2_LIBS)
AC_SUBST(GTK2_CFLAGS)

PKG_CHECK_MODULES(GTK3, gtk+-3.0)
AC_SUBST(GTK3_LIBS)
AC_SUBST(GTK3_CFLAGS)

PKG_CHECK_MODULES(LIBOSSO, libosso >= 0.7.6)
AC_SUBST(LIBOSSO_LIBS)
AC_SUBST(LIBOSSO_CFLAGS)

PKG_CHECK_MODULES(PANGOCAIRO, pangocairo >= 1.16.0)
AC_SUBST(PANGOCAIRO_LIBS)
AC_SUBST(PANGOCAIRO_CFLAGS)

PKG_CHECK_MODULES(XEXTPROTO, xextproto)
AC_SUBST(XEXTPROTO_LIBS)
AC_SUBST(XEXTPROTO_CFLAGS)

PKG_CHECK_MODULES(XTST, xtst)
AC_SUBST(XTST_LIBS)
AC_SUBST(XTST_CFLAGS)

PKG_CHECK_MODULES(X11, x11)
AC_SUBST(X11_LIBS)
AC_SUBST(X11_CFLAGS)

# We need this for our immodule
GTK2_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
AC_SUBST(GTK2_VERSION)
GTK3_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-3.0`
AC_SUBST(GTK3_VERSION)

AM_GLIB_GNU_GETTEXT

localedir=${datadir}/locale
AC_SUBST(localedir)

etcdir=$sysconfdir
echo "Etcdir=$etcdir"
echo 'comp=${prefix}/etc'
if test "$etcdir" = '${prefix}/etc'; then
	etcdir=$prefix"/etc"
	echo "** Modified **"
	echo $etcdir
else
	echo "** Unmodified **"
	echo $etcdir
fi


initdir=$etcdir/osso-af-init/init.d

AH_TEMPLATE([PREFIX], [Intallation path])
AC_DEFINE_UNQUOTED([PREFIX], ["$prefix"])

AC_SUBST(etcdir)
AC_SUBST(initdir)

AC_OUTPUT([Makefile \
	hildon-input-method-framework-3.0.pc \
	hildon-input-method-framework-3-3.0.pc \
	src/Makefile \
	src/gtk2/Makefile \
	src/gtk3/Makefile \
	docs/Makefile \
	debian/hildon-input-method-framework.install \
  ])
