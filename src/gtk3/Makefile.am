# This file is part of hildon-input-method-framework
#
# Copyright (C) 2005-2007 Nokia Corporation.
#
# Contact: Mohammad Anwari <Mohammad.Anwari.com>
#

AM_CPPFLAGS = \
	$(HILDON_IM_CFLAGS)\
	$(GTK3_CFLAGS)\
	$(GCONF_CFLAGS)\
	$(LIBOSSO_CFLAGS) \
	$(LIBOSSOHELP_CFLAGS) \
	$(HILDON_LGPL_CFLAGS) \
	$(XEXTPROTO_CFLAGS) \
	$(X11_CFLAGS) \
	$(XTST_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\"

immoduledir = $(libdir)/gtk-3.0/$(GTK3_VERSION)/immodules
immodule_LTLIBRARIES = hildon-im-module.la

commondir = $(libdir)
common_LTLIBRARIES = libhildon_im_common_3.la

libhildon_im_common_3_la_SOURCES = \
	../hildon-im-common.c \
	../hildon-im-protocol.c \
	../hildon-im-common.h
libhildon_im_common_3_la_LDFLAGS = -Wl,--as-needed -version-info $(LIBVERSION)\
	$(GTK3_LIBS)
libhildon_im_common_3_la_LIBADD = $(X11_LIBS)

hildon_im_module_la_SOURCES = \
	../hildon-im-context.h \
	../hildon-im-context.c \
	../hildon-im-module.c \
	../hildon-im-gtk.h \
	../hildon-im-gtk.c
hildon_im_module_la_LIBADD = \
	$(GTK3_LIBS) $(XEXTPROTO_LIBS) $(X11_LIBS) $(XTST_LIBS) \
	-lhildon_im_common_3
hildon_im_module_la_LDFLAGS = \
	-Wl,--as-needed \
	-rpath $(immoduledir) \
	-avoid-version \
	-module
hildon_im_module_la_CFLAGS = @CFLAGS@ -DG_LOG_DOMAIN=\""hildon-im-module"\"
